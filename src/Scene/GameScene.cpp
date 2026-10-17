#include "GameScene.h"
#include "SceneManager.h"
#include "../Input/Input.h"
#include "../Core/Config.h"
#include "DxLib.h"
#include <cmath>
#include <cstdio>

GameScene::GameScene(SceneManager* mgr)
    : m_mgr(mgr)
{
}

void GameScene::Enter()
{
    m_player.Reset(PLAYER_START_X, GROUND_Y);
    m_enemyManager.Reset(m_player.GetStageData());
    m_invulnerabilityTimer = m_elapsedTime = m_finishTimer = 0.0f;
    m_lives = INITIAL_LIVES;
    m_score = m_hitCount = m_stageStartScore = 0;
    m_remainingTime = STAGE_TIME_LIMIT;
    m_prevRestartKey = CheckHitKey(KEY_INPUT_R) != 0;
    m_isFinished = m_isClear = m_resultRequested = false;
    if (m_mgr)
    {
        SceneContext& context = m_mgr->Context();
        context.resultKind = ResultKind::None;
        context.lastScore = 0.0f;
        context.lastLives = INITIAL_LIVES;
        context.lastDefeated = context.lastEnemyCount = context.lastHits = 0;
        context.lastPlayTime = 0.0f;
        // 現在用意しているのはstage01.tmxの1ステージ。
        context.stageIndex = 0;
        context.stageCount = 1;
    }
}

void GameScene::RestartStage_()
{
    // 得点・残機・合計プレイ時間を保ち、ステージの状態だけを最初に戻す。
    m_stageStartScore = m_score;
    m_player.Reset(PLAYER_START_X, GROUND_Y);
    m_enemyManager.Reset(m_player.GetStageData());
    m_remainingTime = STAGE_TIME_LIMIT;
    m_invulnerabilityTimer = 0.0f;
}

void GameScene::Finish_(bool stageClear)
{
    if (m_isFinished) return;
    m_isFinished = true;
    m_isClear = stageClear;
    m_finishTimer = 0.0f;
    if (m_mgr)
    {
        SceneContext& context = m_mgr->Context();
        context.lastScore = static_cast<float>(m_score);
        context.lastLives = m_lives;
        context.lastDefeated = m_enemyManager.GetDefeatedCount();
        context.lastEnemyCount = m_enemyManager.GetTotalCount();
        context.lastHits = m_hitCount;
        context.lastPlayTime = m_elapsedTime;
        context.resultKind = stageClear ? ResultKind::StageClear : ResultKind::GameOver;
    }
}

void GameScene::Update(float dt, const Input& input)
{
    if (!std::isfinite(dt) || dt <= 0.0f) return;
    if (m_isFinished)
    {
        m_player.UpdateVisualEffects(dt);
        if (m_isClear)
        {
            // 最後に蹴った敵が飛び出す演出だけは継続する。
            m_enemyManager.Update(dt, m_player.GetStageData(), -1, 0.0f,
                m_player.GetAabb(), false);
        }
        m_finishTimer += dt;
        if (m_finishTimer >= 1.2f && !m_resultRequested && m_mgr)
        {
            m_resultRequested = true;
            m_mgr->RequestChange(SceneType::Result);
        }
        return;
    }

    const bool restartKey = CheckHitKey(KEY_INPUT_R) != 0;
    if (restartKey && !m_prevRestartKey)
    {
        Enter();
        return;
    }
    m_prevRestartKey = restartKey;
    // タイムアップを先に確定し、同じフレームの接触で二重に残機を減らさない。
    const float playDt = dt < m_remainingTime ? dt : m_remainingTime;
    m_elapsedTime += playDt;
    m_remainingTime = m_remainingTime > dt ? m_remainingTime - dt : 0.0f;
    if (m_remainingTime <= 0.0f)
    {
        ++m_hitCount;
        --m_lives;
        if (m_lives <= 0)
        {
            m_lives = 0;
            Finish_(false);
        }
        else
        {
            RestartStage_();
        }
        return;
    }
    m_invulnerabilityTimer = m_invulnerabilityTimer > dt
        ? m_invulnerabilityTimer - dt : 0.0f;
    m_player.Update(dt, input);
    const bool hit = m_enemyManager.Update(dt, m_player.GetStageData(),
        m_player.GetLastBumpedObjectId(), m_player.GetX() + m_player.GetWidth() * 0.5f,
        m_player.GetAabb(), m_invulnerabilityTimer <= 0.0f);

    // やり直し前の得点に今回の撃破分を加える。再出現した敵も改めて加点する。
    m_score = m_stageStartScore + m_enemyManager.GetDefeatedCount() * SCORE_PER_ENEMY;
    if (hit)
    {
        ++m_hitCount;
        --m_lives;
        if (m_lives <= 0)
        {
            m_lives = 0;
            Finish_(false);
        }
        else
        {
            m_player.Respawn();
            m_invulnerabilityTimer = 2.0f;
        }
    }
    if (!m_isFinished && m_enemyManager.GetError().empty()
        && m_enemyManager.GetTotalCount() > 0
        && m_enemyManager.GetDefeatedCount() >= m_enemyManager.GetTotalCount())
    {
        Finish_(true);
    }
}

void GameScene::Draw()
{
    int playerAlpha = 255;
    if (m_invulnerabilityTimer > 0.0f)
    {
        playerAlpha = 128;
        // 無敵2秒のうち、最後の0.6秒は0.075秒ごとに半透明／非表示を切り替える。
        const float blinkDuration = 0.6f;
        const float blinkInterval = 0.075f;
        if (m_invulnerabilityTimer <= blinkDuration)
        {
            const float blinkTime = blinkDuration - m_invulnerabilityTimer;
            const int phase = static_cast<int>(blinkTime / blinkInterval);
            if (phase % 2 != 0) playerAlpha = 0;
        }
    }
    m_player.Draw(playerAlpha);
    m_enemyManager.Draw();
    const unsigned int white = GetColor(255, 255, 255);
    DrawFormatString(24, 16, white, "SCORE: %06d   LIVES: %d", m_score, m_lives);
    DrawString(SCREEN_W - 180, 16, "STAGE: 1 / 1", white);
    const int seconds = static_cast<int>(std::ceil(m_remainingTime));
    char timeText[32];
    const int textLength = std::snprintf(timeText, sizeof(timeText), "TIME %03d", seconds);
    const int textWidth = GetDrawStringWidth(timeText, textLength);
    const unsigned int timeColor = m_remainingTime <= 10.0f
        ? GetColor(255, 80, 80) : white;
    DrawString((SCREEN_W - textWidth) / 2, 16, timeText, timeColor);
    DrawFormatString(24, 44, white, "DEFEATED: %d / %d   INVUL: %.1f",
        m_enemyManager.GetDefeatedCount(), m_enemyManager.GetTotalCount(),
        m_invulnerabilityTimer);
    DrawString(24, 72, "ARROWS: MOVE / SHIFT: RUN / SPACE: JUMP / F3: DEBUG / R: RESTART", white);
    if (!m_enemyManager.GetError().empty())
        DrawFormatString(24, 108, GetColor(255, 150, 120), "%s", m_enemyManager.GetError().c_str());
    if (m_isFinished)
    {
        const char* title = m_isClear ? "STAGE CLEAR!" : "GAME OVER";
        DrawString(554, 220, title, GetColor(0, 0, 0));
        DrawString(552, 218, title, GetColor(255, 230, 120));
        DrawString(500, 250, "MOVING TO RESULT...", white);
    }
}

