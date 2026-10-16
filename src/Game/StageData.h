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

struct StageEnemySpawn
{
    std::string name;
    float x = 0.0f;
    float footY = 0.0f;
    int direction = 1;
};

class StageData
{
public:
    bool Load(const char* filename);
    bool IsLoaded() const { return m_loaded; }
    const std::string& GetError() const { return m_error; }
    const std::vector<StageSolid>& GetSolids() const { return m_solids; }
    const std::vector<StageEnemySpawn>& GetEnemySpawns() const { return m_enemySpawns; }
    float GetPlayerX() const { return m_playerX; }
    float GetPlayerFootY() const { return m_playerFootY; }
    float GetGroundY() const { return m_groundY; }

    // dt内の移動を上面・下面・側面との交差で判定する。
    // x/yは当たり判定左上。横方向はscreenWidthごとにループする。
    void Move(float& x, float& y, float& vx, float& vy,
        float width, float height, float dt, float screenWidth,
        bool& grounded, int& bumpedObjectId) const;

private:
    std::vector<StageSolid> m_solids;
    std::vector<StageEnemySpawn> m_enemySpawns;
    std::string m_error;
    float m_playerX = 220.0f;
    float m_playerFootY = 620.0f;
    float m_groundY = 620.0f;
    bool m_loaded = false;
};

