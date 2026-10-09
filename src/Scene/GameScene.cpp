#include "GameScene.h"
#include "SceneManager.h"

#include "../Input/Input.h"
#include "../Audio/Audio.h"
#include "DxLib.h"

GameScene::GameScene(SceneManager* mgr)
    : m_mgr(mgr)
{
}

void GameScene::Enter()
{
    m_distance = 0.0f;
    m_scrollSpeed = BASE_SCROLL_SPEED;
    m_scrollOffset = 0.0f;
    m_elapsedTime = 0.0f;
    m_difficulty = 0.0f;
    m_isGameOver = false;

    m_background.Reset();
    m_player.Reset(PLAYER_START_X, static_cast<float>(GROUND_Y));
    m_obstacleManager.Reset();
}

void GameScene::Update(float dt, const Input& input)
{
    if (m_isGameOver)
    {
        if (input.Triggered(Action::Decide))
        {
            m_mgr->RequestChange(SceneType::Result);
        }
        return;
    }

    UpdateGame_(dt, input);
}

void GameScene::Draw()
{
    m_background.Draw();

    DrawGround_();

    m_obstacleManager.Draw();
    m_player.Draw();

    DrawHud_();

    if (m_isGameOver)
    {
        DrawGameOver_();
    }
}

void GameScene::UpdateGame_(float dt, const Input& input)
{
    m_player.Update(dt, input);

    UpdateDifficulty_(dt);
    UpdateScroll_(dt);

    m_background.Update(dt, m_scrollSpeed);
    m_obstacleManager.Update(dt, m_scrollSpeed, m_difficulty);

    if (m_obstacleManager.CheckCollision(m_player.GetAabb()))
    {
        RequestGameOver_();
    }
}

void GameScene::UpdateDifficulty_(float dt)
{
    m_elapsedTime += dt;

    m_difficulty = m_elapsedTime / MAX_DIFFICULTY_TIME;

    if (m_difficulty > 1.0f)
    {
        m_difficulty = 1.0f;
    }
}

void GameScene::UpdateScroll_(float dt)
{
    m_scrollSpeed = BASE_SCROLL_SPEED + m_elapsedTime * SPEED_UP_RATE;

    if (m_scrollSpeed > MAX_SCROLL_SPEED)
    {
        m_scrollSpeed = MAX_SCROLL_SPEED;
    }

    m_scrollOffset += m_scrollSpeed * dt;

    if (m_scrollOffset >= GROUND_PATTERN_INTERVAL)
    {
        m_scrollOffset -= GROUND_PATTERN_INTERVAL;
    }

    m_distance += m_scrollSpeed * dt;
}

void GameScene::DrawGround_() const
{
    DrawBox(0, GROUND_Y, SCREEN_W, SCREEN_H, GetColor(70, 110, 70), TRUE);
    DrawLine(0, GROUND_Y, SCREEN_W, GROUND_Y, GetColor(255, 255, 255));

    for (float x = -m_scrollOffset; x < SCREEN_W + GROUND_PATTERN_INTERVAL; x += GROUND_PATTERN_INTERVAL)
    {
        const int ix = static_cast<int>(x);
        DrawLine(ix, GROUND_Y, ix + 24, GROUND_Y - 20, GetColor(90, 140, 90));
    }
}

void GameScene::DrawHud_() const
{
    const int white = GetColor(255, 255, 255);
    const int gray = GetColor(190, 190, 190);
    const int panel = GetColor(0, 0, 0);

    DrawBox(18, 18, 300, 116, panel, TRUE);
    DrawFormatString(32, 32, white, "SCORE  %.0f", m_distance);
    DrawFormatString(32, 64, gray, "SPEED  %.0f", m_scrollSpeed);
    DrawFormatString(32, 92, gray, "TIME   %.1f", m_elapsedTime);
}

void GameScene::DrawGameOver_() const
{
    const int panel = GetColor(0, 0, 0);
    const int white = GetColor(255, 255, 255);
    const int red = GetColor(255, 100, 100);
    const int gray = GetColor(200, 200, 200);

    DrawBox(340, 220, 940, 470, panel, TRUE);
    DrawLine(340, 220, 940, 220, red);
    DrawLine(340, 470, 940, 470, red);

    DrawString(545, 260, "GAME OVER", red);
    DrawFormatString(500, 320, white, "FINAL SCORE : %.0f", m_distance);
    DrawString(455, 385, "PRESS DECIDE TO RESULT", gray);
}

void GameScene::RequestGameOver_()
{
    if (m_isGameOver)
    {
        return;
    }

    m_isGameOver = true;

    m_mgr->Context().lastScore = m_distance;
    m_mgr->Context().resultKind = ResultKind::GameOver;
    m_mgr->Context().stageIndex = 0;

    Audio::Instance().PlaySe("hit");
}