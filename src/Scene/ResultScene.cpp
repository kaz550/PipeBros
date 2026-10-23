#include "ResultScene.h"
#include "SceneManager.h"
#include "../Input/Input.h"
#include "../Audio/Audio.h"
#include "../Game/Score.h"
#include "../Core/Config.h"
#include "DxLib.h"
#include <cmath>
#include <cstdio>
#include <cstring>

ResultScene::~ResultScene()
{
    if (DxLib_IsInit() == 0) return;
    if (m_menuFont != -1) DeleteFontToHandle(m_menuFont);
    if (m_smallFont != -1) DeleteFontToHandle(m_smallFont);
    if (m_headerFont != -1) DeleteFontToHandle(m_headerFont);
}

void ResultScene::Enter()
{
    m_selected = 0;
    m_actionRequested = false;
    m_isNew = m_saveFailed = false;
    m_waitDecideRelease = true;
    m_inputLockTimer = 0.25f;
    if (!m_background.Load("assets/stages/stage01.tmx"))
        printfDx("Result background load failed: %s\n", m_background.GetError().c_str());
    if (m_menuFont == -1)
        m_menuFont = CreateFontToHandle("Arial", 26, 7, DX_FONTTYPE_ANTIALIASING_4X4);
    if (m_smallFont == -1)
        m_smallFont = CreateFontToHandle("Arial", 18, 4, DX_FONTTYPE_ANTIALIASING_4X4);
    if (m_headerFont == -1)
        m_headerFont = CreateFontToHandle("Arial", 32, 8, DX_FONTTYPE_ANTIALIASING_4X4);
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
    if (!m_mgr || m_actionRequested || !std::isfinite(dt) || dt <= 0.0f) return;
    if (input.Triggered(Action::ToggleFps))
        m_mgr->Context().showFps = !m_mgr->Context().showFps;
    if (input.Triggered(Action::ForceLow))
        m_mgr->Context().quality = QualityLevel::Low;

    if (!input.Pressed(Action::Decide)) m_waitDecideRelease = false;
    m_inputLockTimer = m_inputLockTimer > dt ? m_inputLockTimer - dt : 0.0f;
    if (m_inputLockTimer > 0.0f) return;
    if (input.Triggered(Action::ToTitle) || input.Triggered(Action::Back))
    {
        m_actionRequested = true;
        Audio::Instance().PlaySe("back");
        m_mgr->RequestChange(SceneType::Title);
        return;
    }
    if (input.Triggered(Action::Up)) m_selected = (m_selected + 1) % 2;
    else if (input.Triggered(Action::Down)) m_selected = (m_selected + 1) % 2;
    if (!m_waitDecideRelease && input.Triggered(Action::Decide))
    {
        m_actionRequested = true;
        Audio::Instance().PlaySe("decide");
        m_mgr->RequestChange(m_selected == 0 ? SceneType::Game : SceneType::Title);
    }
}

void ResultScene::DrawCentered_(const char* text, int y, int color, int font) const
{
    const int length = static_cast<int>(std::strlen(text));
    const int width = font != -1 ? GetDrawStringWidthToHandle(text, length, font)
        : GetDrawStringWidth(text, length);
    const int x = (SCREEN_W - width) / 2;
    if (font != -1) DrawStringToHandle(x, y, text, color, font);
    else DrawString(x, y, text, color);
}

void ResultScene::Draw()
{
    if (!m_mgr) return;
    const SceneContext& context = m_mgr->Context();
    int mode = DX_BLENDMODE_NOBLEND, param = 0;
    GetDrawBlendMode(&mode, &param);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    DrawBox(0, 0, SCREEN_W, SCREEN_H, GetColor(8, 22, 30), TRUE);
    m_background.Draw();
    // HOW TO PLAYと同じ暗幕・パネル・金枠を使う。
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 220);
    DrawBox(0, 0, SCREEN_W, SCREEN_H, GetColor(4, 10, 15), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    DrawBox(270, 132, 1010, 610, GetColor(12, 29, 36), TRUE);
    DrawBox(270, 132, 1010, 610, GetColor(229, 167, 69), FALSE);
    const int white = GetColor(235, 237, 225);
    const int gold = GetColor(255, 201, 96);
    const char* title = "RESULT";
    if (context.resultKind == ResultKind::GameOver) title = "GAME OVER";
    else if (context.resultKind == ResultKind::StageClear) title = "STAGE CLEAR";
    else if (context.resultKind == ResultKind::AllClear) title = "ALL CLEAR";
    DrawCentered_(title, 162, gold, m_headerFont);
    if (m_isNew) DrawCentered_("NEW RECORD!", 200, gold, m_smallFont);
    char text[96];
    if (context.playerCount == 2)
    {
        std::snprintf(text, sizeof(text), "PLAYER 1   %06d          PLAYER 2   %06d",
            context.playerScores[0], context.playerScores[1]);
        DrawCentered_(text, 230, white, m_smallFont);
    }
    else
    {
        std::snprintf(text, sizeof(text), "SCORE    %06.0f", context.lastScore);
        DrawCentered_(text, 226, white, m_menuFont);
    }
    std::snprintf(text, sizeof(text), "HI-SCORE    %06.0f", context.hiScore);
    DrawCentered_(text, 262, gold, m_menuFont);
    if (context.playerCount == 2)
        std::snprintf(text, sizeof(text), "STAGE %02d    P1 LIVES %d    P2 LIVES %d",
            context.stageIndex + 1, context.playerLives[0], context.playerLives[1]);
    else
        std::snprintf(text, sizeof(text), "STAGE %02d     LIVES %d", context.stageIndex + 1, context.lastLives);
    DrawCentered_(text, 306, white, m_smallFont);
    std::snprintf(text, sizeof(text), "DEFEATED    %d / %d", context.lastDefeated, context.lastEnemyCount);
    DrawCentered_(text, 338, white, m_smallFont);
    std::snprintf(text, sizeof(text), "PLAY TIME    %.1f SEC", context.lastPlayTime);
    DrawCentered_(text, 370, white, m_smallFont);
    if (m_saveFailed)
        DrawCentered_("HI-SCORE SAVE FAILED", 400, GetColor(255, 150, 120), m_smallFont);

    const char* labels[] = { "RETRY", "TITLE" };
    for (int i = 0; i < 2; ++i)
    {
        const int y = 434 + i * 56;
        const bool selected = i == m_selected;
        DrawBox(450, y, 830, y + 44, selected ? GetColor(225, 147, 43) : GetColor(12, 31, 38), TRUE);
        DrawBox(450, y, 830, y + 44, selected ? GetColor(255, 201, 96) : GetColor(93, 112, 104), FALSE);
        DrawCentered_(labels[i], y + 8, selected ? GetColor(39, 29, 16) : white, m_menuFont);
        if (selected)
        {
            if (m_menuFont != -1) DrawStringToHandle(470, y + 8, ">", GetColor(39, 29, 16), m_menuFont);
            else DrawString(470, y + 8, ">", GetColor(39, 29, 16));
        }
    }
    DrawCentered_("UP / DOWN: SELECT   ENTER / SPACE: DECIDE", 552, white, m_smallFont);
    DrawCentered_("T: TITLE   ESC: QUIT", 580, gold, m_smallFont);
    SetDrawBlendMode(mode, param);
}
