#include "StageData.h"
#include "../ThirdParty/tinyxml2/tinyxml2.h"
#include "../Core/Config.h"
#include <cmath>
#include <cstring>

bool StageData::Load(const char* filename)
{
    m_loaded = false;
    m_solids.clear();
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

    bool hasPlayer = false;
    bool hasGround = false;
    for (const tinyxml2::XMLElement* group = map->FirstChildElement("objectgroup");
        group; group = group->NextSiblingElement("objectgroup"))
    {
        const char* groupName = group->Attribute("name");
        if (!groupName) continue;
        const bool collisions = std::strcmp(groupName, "Collisions") == 0;
        const bool spawns = std::strcmp(groupName, "Spawns") == 0;
        if (!collisions && !spawns) continue;
        // この段階では、オブジェクト自身に記録した座標だけを扱う。
        if (group->FloatAttribute("offsetx") != 0.0f
            || group->FloatAttribute("offsety") != 0.0f
            || group->FloatAttribute("x") != 0.0f
            || group->FloatAttribute("y") != 0.0f)
        {
            m_error = "Collisions and Spawns must have zero layer offsets.";
            return false;
        }
        for (const tinyxml2::XMLElement* object = group->FirstChildElement("object");
            object; object = object->NextSiblingElement("object"))
        {
            const char* name = object->Attribute("name");
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
    m_loaded = true;
    return true;
}

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


