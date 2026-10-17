#pragma once
#include "IScene.h"
#include "../Game/Player.h"
#include "../Game/EnemyManager.h"

class SceneManager;
class Input;

class GameScene : public IScene
{
public:
    explicit GameScene(SceneManager* mgr);
    void Enter() override;
    void Update(float dt, const Input& input) override;
    void Draw() override;

private:
    void RestartStage_();
    void Finish_(bool stageClear);
    static constexpr float GROUND_Y = 620.0f;
    static constexpr float PLAYER_START_X = 220.0f;
    static constexpr float STAGE_TIME_LIMIT = 120.0f;
    static constexpr int INITIAL_LIVES = 3;
    static constexpr int SCORE_PER_ENEMY = 100;
    SceneManager* m_mgr = nullptr;
    Player m_player;
    EnemyManager m_enemyManager;
    float m_invulnerabilityTimer = 0.0f;
    float m_remainingTime = STAGE_TIME_LIMIT;
    float m_elapsedTime = 0.0f;
    float m_finishTimer = 0.0f;
    int m_lives = INITIAL_LIVES;
    int m_score = 0;
    int m_stageStartScore = 0;
    int m_hitCount = 0;
    bool m_prevRestartKey = false;
    bool m_isFinished = false;
    bool m_isClear = false;
    bool m_resultRequested = false;
};
