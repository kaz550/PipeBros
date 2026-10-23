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
    ~GameScene() override;
    void Enter() override;
    void Update(float dt, const Input& input) override;
    void Draw() override;

private:
    int GetTextWidth_(const char* text, int font) const;
    void DrawText_(int x, int y, const char* text, int color, int font) const;
    void DrawCentered_(const char* text, int y, int color, int font) const;
    static EnemyManager::Lineup GetStageLineup_(int stageNumber);
    static float GetStageTimeLimit_(int stageNumber);
    void RestartStage_();
    void RespawnPlayer_();
    void Finish_(bool stageClear);
    static constexpr float GROUND_Y = 620.0f;
    static constexpr float PLAYER_START_X = 220.0f;
    static constexpr float STAGE_INTRO_DURATION = 1.5f;
    static constexpr float STAGE_CLEAR_DURATION = 1.2f;
    // ミスから復活までの秒数。0.0fなら従来の即時復活。
    static constexpr float RESPAWN_DELAY = 1.0f;
    static constexpr int INITIAL_LIVES = 3;
    static constexpr int SCORE_PER_ENEMY = 100;
    int m_hudFont = -1;
    int m_scoreFont = -1;
    int m_smallFont = -1;
    SceneManager* m_mgr = nullptr;
    Player m_player;
    EnemyManager m_enemyManager;
    float m_invulnerabilityTimer = 0.0f;
    float m_respawnTimer = 0.0f;
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
    bool m_isFinished = false;
    bool m_isClear = false;
    bool m_resultRequested = false;
};
