// 土管兄弟 ソース解説
// TMXから衝突矩形・開始地点・敵の出入口・POW・コイン中心を読む。
// タイルの見た目はStageRenderer。全プレイヤーと敵が同じ地形・POW残数を参照する。

#include "StageData.h"
#include "../ThirdParty/tinyxml2/tinyxml2.h"
#include "../Core/Config.h"
#include <algorithm>
#include <cmath>
#include <cstring>

// POW読込の共通入口。表示と判定が同じ矩形を使うため、StageDataとStageRendererから呼ぶ。
bool ReadStagePow(const tinyxml2::XMLElement* map, StageSolid& pow, std::string& error)
{
    pow = StageSolid{};
    for (const tinyxml2::XMLElement* group = map->FirstChildElement("objectgroup");
        group; group = group->NextSiblingElement("objectgroup"))
    {
        const char* name = group->Attribute("name");
        if (!name || std::strcmp(name, "Objects") != 0
            || !group->BoolAttribute("visible", true)) continue;
        for (const tinyxml2::XMLElement* object = group->FirstChildElement("object");
            object; object = object->NextSiblingElement("object"))
        {
            const char* type = object->Attribute("class");
            if (!type || !type[0]) type = object->Attribute("type");
            if (!type || std::strcmp(type, "POW") != 0
                || !object->BoolAttribute("visible", true)) continue;
            if (pow.id != -1)
            {
                error = "Only one visible POW is supported.";
                return false;
            }
            StageSolid candidate;
            candidate.id = object->IntAttribute("id", -1);
            if (candidate.id <= 0
                || object->QueryFloatAttribute("x", &candidate.x) != tinyxml2::XML_SUCCESS
                || object->QueryFloatAttribute("y", &candidate.y) != tinyxml2::XML_SUCCESS
                || object->QueryFloatAttribute("width", &candidate.width) != tinyxml2::XML_SUCCESS
                || object->QueryFloatAttribute("height", &candidate.height) != tinyxml2::XML_SUCCESS
                || !std::isfinite(candidate.x) || !std::isfinite(candidate.y)
                || !std::isfinite(candidate.width) || !std::isfinite(candidate.height)
                || candidate.width <= 0.0f || candidate.height <= 0.0f
                || object->FloatAttribute("rotation") != 0.0f || object->Attribute("template")
                || object->Attribute("gid") || object->FirstChildElement("ellipse")
                || object->FirstChildElement("polygon") || object->FirstChildElement("polyline")
                || object->FirstChildElement("point")
                || group->FloatAttribute("offsetx") != 0.0f
                || group->FloatAttribute("offsety") != 0.0f
                || group->FloatAttribute("x") != 0.0f || group->FloatAttribute("y") != 0.0f
                || group->FloatAttribute("opacity", 1.0f) != 1.0f || group->Attribute("tintcolor")
                || group->FloatAttribute("parallaxx", 1.0f) != 1.0f
                || group->FloatAttribute("parallaxy", 1.0f) != 1.0f)
            {
                error = "POW must be an unrotated rectangle with positive size in an unmodified Objects layer.";
                return false;
            }
            // 下面から叩いたとき、POWのIDをPlayerへ通知する。
            candidate.bumpable = true;
            pow = candidate;
        }
    }
    return true;
}

// 前の面の状態を消し、TMXの判定とポイントを検証して読む。最後まで成功した場合だけm_loaded=trueにする。
bool StageData::Load(const char* filename)
{
    m_loaded = false;
    m_pow = StageSolid{};
    m_powUses = 0;
    m_powOriginalHeight = 0.0f;
    m_solids.clear();
    m_bonusCoins.clear();
    m_enemySpawns.clear();
    m_enemyExits.clear();
    m_error.clear();
    tinyxml2::XMLDocument document;
    if (document.LoadFile(filename) != tinyxml2::XML_SUCCESS)
    {
        m_error = document.ErrorStr();
        return false;
    }
    const tinyxml2::XMLElement* map = document.FirstChildElement("map");
    const char* orientation = map ? map->Attribute("orientation") : nullptr;
    if (!map || !orientation || std::strcmp(orientation, "orthogonal") != 0
        || map->IntAttribute("infinite") != 0
        || map->IntAttribute("width") * map->IntAttribute("tilewidth") != SCREEN_W
        || map->IntAttribute("height") * map->IntAttribute("tileheight") < SCREEN_H)
    {
        m_error = "Expected a finite orthogonal map matching the screen width.";
        return false;
    }

    // 判定に必要な矩形と、スポーン/コインのポイントだけを読む。タイル画像は別担当。
    bool hasRespawn = false;
    bool hasPlayer = false;
    bool hasGround = false;
    for (const tinyxml2::XMLElement* group = map->FirstChildElement("objectgroup");
        group; group = group->NextSiblingElement("objectgroup"))
    {
        const char* groupName = group->Attribute("name");
        if (!groupName) continue;
        const bool collisions = std::strcmp(groupName, "Collisions") == 0;
        const bool spawns = std::strcmp(groupName, "Spawns") == 0;
        const bool bonusCoins = std::strcmp(groupName, "BonusCoins") == 0;
        if (!collisions && !spawns && !bonusCoins) continue;
        // この段階では、オブジェクト自身に記録した座標だけを扱う。
        if (group->FloatAttribute("offsetx") != 0.0f
            || group->FloatAttribute("offsety") != 0.0f
            || group->FloatAttribute("x") != 0.0f
            || group->FloatAttribute("y") != 0.0f)
        {
            m_error = "Collisions, Spawns and BonusCoins must have zero layer offsets.";
            return false;
        }
        for (const tinyxml2::XMLElement* object = group->FirstChildElement("object");
            object; object = object->NextSiblingElement("object"))
        {
            const char* name = object->Attribute("name");
            if (bonusCoins)
            {
                StageBonusCoin coin;
                if (!object->FirstChildElement("point") || object->Attribute("gid")
                    || object->FloatAttribute("rotation") != 0.0f
                    || object->QueryFloatAttribute("x", &coin.x) != tinyxml2::XML_SUCCESS
                    || object->QueryFloatAttribute("y", &coin.y) != tinyxml2::XML_SUCCESS
                    || !std::isfinite(coin.x) || !std::isfinite(coin.y)
                    || coin.x < 0 || coin.x >= SCREEN_W || coin.y < 0 || coin.y >= SCREEN_H)
                {
                    m_error = "BonusCoins must contain unrotated points inside the screen.";
                    return false;
                }
                m_bonusCoins.push_back(coin);
                continue;
            }
            if (spawns)
            {
                const char* objectType = object->Attribute("class");
                if (!objectType || objectType[0] == '\0') objectType = object->Attribute("type");
                if (objectType && (std::strcmp(objectType, "EnemySpawn") == 0
                    || std::strcmp(objectType, "EnemyExit") == 0))
                {
                    const bool isExit = std::strcmp(objectType, "EnemyExit") == 0;
                    StageEnemySpawn spawn;
                    spawn.name = name ? name : "EnemySpawn";
                    if (!object->FirstChildElement("point")
                        || object->QueryFloatAttribute("x", &spawn.x) != tinyxml2::XML_SUCCESS
                        || object->QueryFloatAttribute("y", &spawn.footY) != tinyxml2::XML_SUCCESS
                        || !std::isfinite(spawn.x) || !std::isfinite(spawn.footY))
                    {
                        m_error = "EnemySpawn/EnemyExit must be a point with valid x/y coordinates.";
                        return false;
                    }
                    spawn.direction = spawn.x < SCREEN_W * 0.5f ? 1 : -1;
                    if (isExit) spawn.direction = -spawn.direction;
                    const tinyxml2::XMLElement* properties = object->FirstChildElement("properties");
                    if (properties)
                    {
                        for (const tinyxml2::XMLElement* property = properties->FirstChildElement("property");
                            property; property = property->NextSiblingElement("property"))
                        {
                            const char* key = property->Attribute("name");
                            if (!isExit && key && std::strcmp(key, "enemyType") == 0)
                            {
                                const char* value = property->Attribute("value");
                                if (!value || (std::strcmp(value, "turtle") != 0 && std::strcmp(value, "crab") != 0))
                                {
                                    m_error = "EnemySpawn enemyType must be turtle or crab.";
                                    return false;
                                }
                                spawn.enemyType = value;
                            }
                            if (key && std::strcmp(key, "direction") == 0)
                            {
                                if (property->QueryIntAttribute("value", &spawn.direction) != tinyxml2::XML_SUCCESS
                                    || (spawn.direction != 1 && spawn.direction != -1))
                                {
                                    m_error = "EnemySpawn/EnemyExit direction must be 1 or -1.";
                                    return false;
                                }
                            }
                        }
                    }
                    if (isExit)
                    {
                        StageEnemyExit exit;
                        exit.name = spawn.name;
                        exit.x = spawn.x;
                        exit.footY = spawn.footY;
                        exit.direction = spawn.direction;
                        m_enemyExits.push_back(exit);
                    }
                    else m_enemySpawns.push_back(spawn);
                    continue;
                }
                if ((objectType && std::strcmp(objectType, "PlayerRespawn") == 0)
                    || (name && std::strcmp(name, "PlayerRespawn") == 0))
                {
                    if (hasRespawn || !object->FirstChildElement("point")
                        || object->QueryFloatAttribute("x", &m_playerRespawnCenterX) != tinyxml2::XML_SUCCESS
                        || object->QueryFloatAttribute("y", &m_playerRespawnFootY) != tinyxml2::XML_SUCCESS
                        || !std::isfinite(m_playerRespawnCenterX) || !std::isfinite(m_playerRespawnFootY))
                    {
                        m_error = "PlayerRespawn must be a single point with valid x/y coordinates.";
                        return false;
                    }
                    hasRespawn = true;
                    continue;
                }
                if (!name || std::strcmp(name, "Player1") != 0) continue;
                if (hasPlayer || !object->FirstChildElement("point")
                    || object->QueryFloatAttribute("x", &m_playerX) != tinyxml2::XML_SUCCESS
                    || object->QueryFloatAttribute("y", &m_playerFootY) != tinyxml2::XML_SUCCESS
                    || !std::isfinite(m_playerX) || !std::isfinite(m_playerFootY))
                {
                    m_error = "Spawns needs exactly one valid Player1 point.";
                    return false;
                }
                hasPlayer = true;
                continue;
            }
            StageSolid solid;
            solid.id = object->IntAttribute("id", -1);
            if (object->QueryFloatAttribute("x", &solid.x) != tinyxml2::XML_SUCCESS
                || object->QueryFloatAttribute("y", &solid.y) != tinyxml2::XML_SUCCESS
                || object->QueryFloatAttribute("width", &solid.width) != tinyxml2::XML_SUCCESS
                || object->QueryFloatAttribute("height", &solid.height) != tinyxml2::XML_SUCCESS
                || !std::isfinite(solid.x) || !std::isfinite(solid.y)
                || !std::isfinite(solid.width) || !std::isfinite(solid.height)
                || solid.width <= 0.0f || solid.height <= 0.0f || solid.id <= 0
                || object->FloatAttribute("rotation") != 0.0f
                || object->FirstChildElement("ellipse")
                || object->FirstChildElement("polygon")
                || object->FirstChildElement("polyline")
                || object->FirstChildElement("point") || object->Attribute("gid"))
            {
                m_error = "Collisions must contain unrotated rectangles with positive sizes.";
                return false;
            }
            const tinyxml2::XMLElement* properties = object->FirstChildElement("properties");
            if (properties)
            {
                for (const tinyxml2::XMLElement* property = properties->FirstChildElement("property");
                    property; property = property->NextSiblingElement("property"))
                {
                    const char* key = property->Attribute("name");
                    if (key && std::strcmp(key, "bumpable") == 0)
                        solid.bumpable = property->BoolAttribute("value");
                }
            }
            if (name && std::strcmp(name, "Ground") == 0)
            {
                m_groundY = solid.y;
                hasGround = true;
            }
            m_solids.push_back(solid);
        }
    }
    if (!hasPlayer || !hasGround)
    {
        m_error = "Missing Spawns/Player1 or Collisions/Ground.";
        return false;
    }
    if (!hasRespawn)
    {
        // 追加ポイントがない既存マップでも、下段中央に再出現する。
        m_playerRespawnCenterX = SCREEN_W * 0.5f;
        m_playerRespawnFootY = m_groundY;
    }
    if (!ReadStagePow(map, m_pow, m_error)) return false;
    if (m_pow.id != -1)
    {
        for (const StageSolid& solid : m_solids)
        {
            if (solid.id == m_pow.id)
            {
                m_error = "POW and collision objects must have unique IDs.";
                return false;
            }
        }
        m_solids.push_back(m_pow);
        m_powUses = MAX_POW_USES;
        m_powOriginalHeight = m_pow.height;
    }
    m_loaded = true;
    return true;
}

// 共通の残り回数を1つ減らす。下面を固定して薄くし、0なら衝突矩形も取り除く。
bool StageData::UsePow()
{
    if (!m_loaded || m_powUses <= 0) return false;
    --m_powUses;
    if (m_powUses == 0)
    {
        // 最後の衝撃を処理した後、足場そのものを取り除く。
        m_pow.bumpable = false;
        m_solids.erase(std::remove_if(m_solids.begin(), m_solids.end(),
            [this](const StageSolid& solid) { return solid.id == m_pow.id; }), m_solids.end());
    }
    else
    {
        // 下面の高さは固定。残り3/2/1回で高さを100/75/50%へ縮める。
        const float bottom = m_pow.y + m_pow.height;
        m_pow.height = m_powOriginalHeight * (m_powUses + 1) / (MAX_POW_USES + 1);
        m_pow.y = bottom - m_pow.height;
        for (StageSolid& solid : m_solids)
        {
            if (solid.id == m_pow.id)
            {
                solid = m_pow;
                break;
            }
        }
    }
    return true;
}

// 移動区間を小分けにし、X→Yの順で矩形の側面・上面・下面との交差を解決する。位置・速度・接地・叩いたIDを参照引数で返す。
void StageData::Move(float& x, float& y, float& vx, float& vy,
    float width, float height, float dt, float screenWidth,
    bool& grounded, int& bumpedObjectId) const
{
    if (!m_loaded || dt <= 0.0f || screenWidth <= 0.0f) return;
    bumpedObjectId = -1;
    grounded = false;
    // 移動を小分けにし、画面をまたぐ際も近隣の足場を判定する。
    const float totalX = vx * dt;
    const float totalY = vy * dt;
    const float maxDistance = std::fmax(std::fabs(totalX), std::fabs(totalY));
    const int steps = static_cast<int>(std::ceil(maxDistance / 16.0f)) + 1;
    const float stepTime = dt / steps;
    for (int step = 0; step < steps; ++step)
    {
        const float oldX = x;
        float nextX = x + vx * stepTime;
        bool hitSide = false;
        for (const StageSolid& solid : m_solids)
        {
            if (y + height <= solid.y || y >= solid.y + solid.height) continue;
            for (int copy = -1; copy <= 1; ++copy)
            {
                const float left = solid.x + copy * screenWidth;
                const float right = left + solid.width;
                if (vx > 0.0f && oldX + width <= left && nextX + width >= left)
                {
                    nextX = left - width;
                    hitSide = true;
                }
                else if (vx < 0.0f && oldX >= right && nextX <= right)
                {
                    nextX = right;
                    hitSide = true;
                }
            }
        }
        x = nextX;
        if (hitSide) vx = 0.0f;
        while (x + width * 0.5f < 0.0f) x += screenWidth;
        while (x + width * 0.5f >= screenWidth) x -= screenWidth;

        // 横方向を確定してから縦を解決。落下は上面、上昇は下面を横切ったときだけ止める。
        const float oldY = y;
        float nextY = y + vy * stepTime;
        bool hitVertical = false;
        bool onSurface = false;
        int hitId = -1;
        for (const StageSolid& solid : m_solids)
        {
            bool overlapsX = false;
            for (int copy = -1; copy <= 1; ++copy)
            {
                const float left = solid.x + copy * screenWidth;
                if (x + width > left && x < left + solid.width) overlapsX = true;
            }
            if (!overlapsX) continue;
            const float top = solid.y;
            const float bottom = solid.y + solid.height;
            if (vy >= 0.0f && oldY + height <= top && nextY + height >= top)
            {
                nextY = top - height;
                hitVertical = true;
                onSurface = true;
                hitId = -1;
            }
            else if (vy < 0.0f && oldY >= bottom && nextY <= bottom)
            {
                nextY = bottom;
                hitVertical = true;
                onSurface = false;
                hitId = solid.bumpable ? solid.id : -1;
            }
        }
        y = nextY;
        grounded = onSurface;
        if (hitVertical)
        {
            vy = 0.0f;
            if (hitId != -1) bumpedObjectId = hitId;
        }
    }
}
