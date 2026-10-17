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
    static float GetStageTimeLimit_(int stageNumber);
    void RestartStage_();
    void Finish_(bool stageClear);
    static constexpr float GROUND_Y = 620.0f;
    static constexpr float PLAYER_START_X = 220.0f;
    static constexpr float STAGE_INTRO_DURATION = 1.5f;
    static constexpr float STAGE_CLEAR_DURATION = 1.2f;
    static constexpr int INITIAL_LIVES = 3;
    static constexpr int SCORE_PER_ENEMY = 100;
    SceneManager* m_mgr = nullptr;
    Player m_player;
    EnemyManager m_enemyManager;
    float m_invulnerabilityTimer = 0.0f;
    // 0秒なら制限なし。正の値なら残り時間表示とタイムアップ処理を有効にする。
    float m_stageTimeLimit = 0.0f;
    float m_remainingTime = 0.0f;
    float m_elapsedTime = 0.0f;
    float m_finishTimer = 0.0f;
    float m_stageIntroTimer = STAGE_INTRO_DURATION;
    int m_stageNumber = 1;
    int m_lives = INITIAL_LIVES;
    int m_score = 0;
    int m_stageStartScore = 0;
    int m_hitCount = 0;
    bool m_prevRestartKey = false;
    bool m_isFinished = false;
    bool m_isClear = false;
    bool m_resultRequested = false;
};
