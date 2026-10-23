#include "ResultScene.h"
#include "SceneManager.h"
#include "../Input/Input.h"
#include "../Audio/Audio.h"
#include "../Game/Score.h"
#include "DxLib.h"
#include <cmath>

void ResultScene::Enter()
{
    m_isNew = m_saveFailed = false;
    m_waitDecideRelease = true;
    m_inputLockTimer = 0.25f;
    if (!m_background.Load("assets/stages/stage01.tmx"))
        printfDx("Result background load failed: %s\n", m_background.GetError().c_str());
    if (!m_mgr) return;
    SceneContext& context = m_mgr->Context();
    context.lastScore = Score::ClampScore(context.lastScore);
    if (context.lastScore > context.hiScore)
    {
        context.hiScore = context.lastScore;
        m_saveFailed = !Score::SaveHiScore("hiscore.dat", context.hiScore);
        m_isNew = true;
    }
}

void ResultScene::Update(float dt, const Input& input)
{
    if (!m_mgr || !std::isfinite(dt) || dt <= 0.0f) return;
    if (input.Triggered(Action::ToggleFps))
        m_mgr->Context().showFps = !m_mgr->Context().showFps;
    if (input.Triggered(Action::ForceLow))
        m_mgr->Context().quality = QualityLevel::Low;

    if (!input.Pressed(Action::Decide)) m_waitDecideRelease = false;
    m_inputLockTimer = m_inputLockTimer > dt ? m_inputLockTimer - dt : 0.0f;
    if (m_inputLockTimer > 0.0f) return;
    if (input.Triggered(Action::ToTitle) || input.Triggered(Action::Back))
    {
        Audio::Instance().PlaySe("back");
        m_mgr->RequestChange(SceneType::Title);
        return;
    }
    if (!m_waitDecideRelease && input.Triggered(Action::Decide))
    {
        Audio::Instance().PlaySe("decide");
        m_mgr->RequestChange(SceneType::Game);
    }
}

void ResultScene::Draw()
{
    m_background.Draw();
    if (!m_mgr) return;
    const SceneContext& context = m_mgr->Context();
    const unsigned int white = GetColor(255, 255, 255);
    const unsigned int gold = GetColor(255, 230, 120);
    const char* title = "RESULT";
    if (context.resultKind == ResultKind::StageClear) title = "STAGE CLEAR";
    else if (context.resultKind == ResultKind::GameOver) title = "GAME OVER";
    else if (context.resultKind == ResultKind::AllClear) title = "ALL CLEAR";
    DrawString(514, 220, title, GetColor(0, 0, 0));
    DrawString(512, 218, title, gold);
    DrawFormatString(512, 250, white, "SCORE:    %.0f", context.lastScore);
    DrawFormatString(512, 278, white, "HI-SCORE: %.0f", context.hiScore);
    DrawFormatString(512, 362, white, "LIVES LEFT: %d", context.lastLives);
    DrawFormatString(512, 390, white, "DEFEATED:   %d / %d", context.lastDefeated, context.lastEnemyCount);
    DrawFormatString(512, 418, white, "TIME:       %.1f SEC", context.lastPlayTime);
    if (m_isNew) DrawString(512, 522, "NEW RECORD!", gold);
    DrawString(448, 554, "ENTER / SPACE: RETRY", white);
    DrawString(448, 582, "T: TITLE   /   ESC: QUIT", white);
    if (m_saveFailed)
        DrawString(24, 108, "HI-SCORE SAVE FAILED", GetColor(255, 150, 120));
}
