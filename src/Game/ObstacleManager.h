#pragma once
#include "Obstacle.h"
#include "Aabb.h"
#include <vector>

class ObstacleManager
{
public:
    void Reset();
    void Update(float dt, float scrollSpeed, float difficulty);
    void Draw() const;

    bool CheckCollision(const Aabb& playerAabb) const;

    int GetObstacleCount() const { return static_cast<int>(m_obstacles.size()); }

private:
    void SpawnObstacle_();
    void RemoveDeadObstacles_();
    float CreateNextSpawnInterval_(float difficulty) const;
    float CreateRandomHeight_(float difficulty) const;

private:
    static constexpr float SCREEN_W = 1280.0f;
    static constexpr float GROUND_Y = 620.0f;
    static constexpr float OBSTACLE_WIDTH = 48.0f;

private:
    std::vector<Obstacle> m_obstacles;

    float m_spawnTimer = 0.0f;
    float m_nextSpawnInterval = 1.0f;
    float m_currentDifficulty = 0.0f;
};