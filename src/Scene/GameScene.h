// 土管兄弟 ソース解説
// 共通ステージと2人分の状態を所有し、得点・残機・面進行・ボーナスを統括する。
// Playerは各個体、EnemyManagerは敵全体を更新する。待ち時間と接触の順序もこのシーンで制御する。

#pragma once
#include "IScene.h"
#include <string>
#include <vector>
#include "../Game/Player.h"
#include "../Game/EnemyManager.h"
#include "../Game/StageRenderer.h"

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
    struct BonusCoin { float x, y; bool collected = false; };
    // 個人の残機・得点・復帰/無敵時計。hits/defeatedの累積は現状の表示では未使用。
    struct PlayerState
    {
        int lives = 0, score = 0, hits = 0, defeated = 0;
        float invulnerability = 0.0f, respawn = 0.0f;
    };
    bool HasLivingPlayer_() const;
    void AddScore_(int playerIndex, int points);
    void DrawStageDebug_() const;
    bool LoadBonusSprite_();
    void UpdateBonus_(float dt, const Input& input);
    void DrawBonus_() const;
    void UpdateStageSelect_(const Input& input);
    void DrawStageSelect_() const;
    static const char* GetStageKindName_(int stageNumber);
    int GetTextWidth_(const char* text, int font) const;
    void DrawText_(int x, int y, const char* text, int color, int font) const;
    void DrawCentered_(const char* text, int y, int color, int font) const;
    static EnemyManager::Lineup GetStageLineup_(int stageNumber);
    static float GetStageTimeLimit_(int stageNumber);
    void RestartStage_();
    void RespawnPlayer_(int playerIndex);
    void Finish_(bool stageClear);
    // 時間の単位は秒。開始・クリア・復帰の待機は別々の状態として停止/再開する。
    static constexpr float STAGE_INTRO_DURATION = 1.5f;
    static constexpr float STAGE_CLEAR_DURATION = 1.2f;
    // ミスから復活までの秒数。0.0fなら従来の即時復活。
    static constexpr float RESPAWN_DELAY = 1.0f;
    static constexpr float BONUS_TIME_LIMIT = 20.0f;
    static constexpr int BONUS_COIN_SCORE = 100;
    static constexpr int BONUS_PERFECT_SCORE = 1000;
    static constexpr float BONUS_COIN_SIZE = 24.0f;
    static constexpr int DEBUG_MAX_STAGE = 99;
    static constexpr int INITIAL_LIVES = 3;
    static constexpr int SCORE_PER_ENEMY = 100;
    std::vector<BonusCoin> m_bonusCoins;
    std::string m_bonusError;
    int m_coinGraph = -1;
    int m_bonusCollected = 0;
    float m_bonusAnimationTime = 0.0f;
    bool m_isBonus = false;
    bool m_stageSelectOpen = false;
    bool m_waitStageSelectRelease = true;
    int m_selectedStage = 1;
    int m_hudFont = -1;
    int m_scoreFont = -1;
    int m_smallFont = -1;
    SceneManager* m_mgr = nullptr;
    // ステージとPOWは全員で共有。描画・演出も1回だけ更新する。
    StageData m_stage;
    StageRenderer m_stageRenderer;
    // 固定2枠のうちm_playerCount人だけを更新。配列の0番=1P、1番=2P。
    Player m_players[2];
    PlayerState m_playerStates[2];
    int m_playerCount = 1;
    bool m_showStageDebug = false;
    std::string m_stageError;
    EnemyManager m_enemyManager;
    // 0秒なら制限なし。正の値なら残り時間表示とタイムアップ処理を有効にする。
    float m_stageTimeLimit = 0.0f;
    float m_remainingTime = 0.0f;
    float m_elapsedTime = 0.0f;
    float m_finishTimer = 0.0f;
    float m_stageIntroTimer = STAGE_INTRO_DURATION;
    int m_stageNumber = 1;
    bool m_isFinished = false;
    bool m_isClear = false;
    bool m_resultRequested = false;
};
