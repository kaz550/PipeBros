#include "GameScene.h"
#include "../Input/Input.h"
#include "DxLib.h"
#include <cmath>

GameScene::GameScene(SceneManager* mgr)
{
    (void)mgr;
}

void GameScene::Enter()
{
    m_player.Reset(PLAYER_START_X, GROUND_Y);
    m_enemyManager.Reset(m_player.GetStageData());
    m_invulnerabilityTimer = 0.0f;
    m_hitCount = 0;
    m_prevRestartKey = CheckHitKey(KEY_INPUT_R) != 0;
}

void GameScene::Update(float dt, const Input& input)
{
    if (!std::isfinite(dt) || dt <= 0.0f) return;
    const bool restartKey = CheckHitKey(KEY_INPUT_R) != 0;
    if (restartKey && !m_prevRestartKey)
    {
        Enter();
        return;
    }
    m_prevRestartKey = restartKey;
    m_invulnerabilityTimer = m_invulnerabilityTimer > dt
        ? m_invulnerabilityTimer - dt : 0.0f;
    m_player.Update(dt, input);
    const bool hit = m_enemyManager.Update(dt, m_player.GetStageData(),
        m_player.GetLastBumpedObjectId(), m_player.GetX() + m_player.GetWidth() * 0.5f,
        m_player.GetAabb(), m_invulnerabilityTimer <= 0.0f);
    if (hit)
    {
        ++m_hitCount;
        m_player.Respawn();
        m_invulnerabilityTimer = 2.0f;
    }
}

void GameScene::Draw()
{
    m_player.Draw();
    m_enemyManager.Draw();
    const unsigned int white = GetColor(255, 255, 255);
    const unsigned int gray = GetColor(180, 200, 210);
    DrawString(24, 24, "PIPE BROS - ENEMY TEST", white);
    DrawString(24, 52, "ARROWS: MOVE / SHIFT: RUN / SPACE: JUMP / F3: DEBUG / R: RESTART", white);
    DrawFormatString(24, 80, white, "DEFEATED: %d / %d   HITS: %d   INVUL: %.1f",
        m_enemyManager.GetDefeatedCount(), m_enemyManager.GetTotalCount(),
        m_hitCount, m_invulnerabilityTimer);
    if (!m_enemyManager.GetError().empty())
        DrawFormatString(24, 108, GetColor(255, 150, 120), "%s", m_enemyManager.GetError().c_str());
    else if (m_enemyManager.GetTotalCount() > 0
        && m_enemyManager.GetDefeatedCount() == m_enemyManager.GetTotalCount())
        DrawString(24, 108, "ALL ENEMIES DEFEATED!  R: RESTART", GetColor(255, 220, 100));
    else
        DrawString(24, 108, "HIT THE PLATFORM FROM BELOW, THEN TOUCH THE OVERTURNED TURTLE.", gray);
}
