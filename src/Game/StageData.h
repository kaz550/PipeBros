#pragma once
#include <string>
#include <vector>

struct StageSolid
{
    int id = -1;
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    bool bumpable = false;
};

// 表示と当たり判定で同じPOW矩形を使用する。未配置ならidは-1。
namespace tinyxml2 { class XMLElement; }
bool ReadStagePow(const tinyxml2::XMLElement* map, StageSolid& pow, std::string& error);

struct StageEnemySpawn
{
    std::string name;
    std::string enemyType = "turtle";
    float x = 0.0f;
    float footY = 0.0f;
    int direction = 1;
};

struct StageEnemyExit
{
    std::string name;
    float x = 0.0f;
    float footY = 0.0f;
    int direction = -1;
};

class StageData
{
public:
    static constexpr int MAX_POW_USES = 3;
    bool Load(const char* filename);
    bool IsLoaded() const { return m_loaded; }
    const std::string& GetError() const { return m_error; }
    const std::vector<StageSolid>& GetSolids() const { return m_solids; }
    const std::vector<StageEnemySpawn>& GetEnemySpawns() const { return m_enemySpawns; }
    const std::vector<StageEnemyExit>& GetEnemyExits() const { return m_enemyExits; }
    float GetPlayerX() const { return m_playerX; }
    float GetPlayerFootY() const { return m_playerFootY; }
    // 再出現ポイントは横方向の中心と足元の座標。
    float GetPlayerRespawnCenterX() const { return m_playerRespawnCenterX; }
    float GetPlayerRespawnFootY() const { return m_playerRespawnFootY; }
    const StageSolid& GetPow() const { return m_pow; }
    int GetPowUses() const { return m_powUses; }
    // 衝撃を敵へ適用した後に1回消費する。未配置・使用済みならfalse。
    bool UsePow();
    float GetGroundY() const { return m_groundY; }

    // dt内の移動を上面・下面・側面との交差で判定する。
    // x/yは当たり判定左上。横方向はscreenWidthごとにループする。
    void Move(float& x, float& y, float& vx, float& vy,
        float width, float height, float dt, float screenWidth,
        bool& grounded, int& bumpedObjectId) const;

private:
    std::vector<StageSolid> m_solids;
    std::vector<StageEnemySpawn> m_enemySpawns;
    std::vector<StageEnemyExit> m_enemyExits;
    StageSolid m_pow;
    int m_powUses = 0;
    float m_powOriginalHeight = 0.0f;
    std::string m_error;
    float m_playerX = 220.0f;
    float m_playerFootY = 620.0f;
    float m_groundY = 620.0f;
    float m_playerRespawnCenterX = 640.0f;
    float m_playerRespawnFootY = 620.0f;
    bool m_loaded = false;
};
