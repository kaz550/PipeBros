#include "ObstacleManager.h"
#include "DxLib.h"
#include <algorithm>

void ObstacleManager::Reset()
{
    m_obstacles.clear();
    m_spawnTimer = 0.0f;
    m_currentDifficulty = 0.0f;
    m_nextSpawnInterval = CreateNextSpawnInterval_(m_currentDifficulty);
}

void ObstacleManager::Update(float dt, float scrollSpeed, float difficulty)
{
    m_currentDifficulty = difficulty;
    m_spawnTimer += dt;

    if (m_spawnTimer >= m_nextSpawnInterval)
    {
        SpawnObstacle_();
        m_spawnTimer = 0.0f;
        m_nextSpawnInterval = CreateNextSpawnInterval_(m_currentDifficulty);
    }

    for (auto& obstacle : m_obstacles)
    {
        obstacle.Update(dt, scrollSpeed);
    }

    RemoveDeadObstacles_();
}

void ObstacleManager::Draw() const
{
    for (const auto& obstacle : m_obstacles)
    {
        obstacle.Draw();
    }
}

bool ObstacleManager::CheckCollision(const Aabb& playerAabb) const
{
    for (const auto& obstacle : m_obstacles)
    {
        if (IsHitAabb(playerAabb, obstacle.GetAabb()))
        {
            return true;
        }
    }

    return false;
}

void ObstacleManager::SpawnObstacle_()
{
    const float height = CreateRandomHeight_(m_currentDifficulty);
    const float x = SCREEN_W + 40.0f;

    m_obstacles.emplace_back(x, GROUND_Y, OBSTACLE_WIDTH, height);
}

void ObstacleManager::RemoveDeadObstacles_()
{
    m_obstacles.erase(
        std::remove_if(
            m_obstacles.begin(),
            m_obstacles.end(),
            [](const Obstacle& obstacle)
            {
                return !obstacle.IsAlive();
            }
        ),
        m_obstacles.end()
    );
}

float ObstacleManager::CreateNextSpawnInterval_(float difficulty) const
{
    // 難易度が上がるほど、少しずつ出現間隔を短くする
    float baseMin = 1.00f - difficulty * 0.20f;
    float baseMax = 1.70f - difficulty * 0.30f;

    if (baseMin < 0.65f)
    {
        baseMin = 0.65f;
    }

    if (baseMax < 1.05f)
    {
        baseMax = 1.05f;
    }

    const int randomValue = GetRand(100);
    const float rate = static_cast<float>(randomValue) / 100.0f;

    return baseMin + (baseMax - baseMin) * rate;
}

float ObstacleManager::CreateRandomHeight_(float difficulty) const
{
    // 難易度が上がるほど、少しだけ高い障害物も出やすくする
    float minHeight = 40.0f;
    float maxHeight = 100.0f + difficulty * 30.0f;

    if (maxHeight > 140.0f)
    {
        maxHeight = 140.0f;
    }

    const int range = static_cast<int>(maxHeight - minHeight);
    return minHeight + static_cast<float>(GetRand(range));
}