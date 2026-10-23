// 土管兄弟 ソース解説
// 背景画像、人数選択、操作説明、終了メニューを表示する。
// 人数はSceneContextへ保存する。メニュー決定の長押しを次画面へ持ち越さない。

#include "TitleScene.h"
#include "SceneManager.h"
#include "SceneType.h"
#include "../Input/Input.h"
#include "../Audio/Audio.h"
#include "../Core/Config.h"
#include "DxLib.h"
#include <cmath>
#include <cstdio>
#include <cstring>

TitleScene::~TitleScene()
{
    if (DxLib_IsInit() == 0) return;
    if (m_background != -1) DeleteGraph(m_background);
    if (m_menuFont != -1) DeleteFontToHandle(m_menuFont);
    if (m_smallFont != -1) DeleteFontToHandle(m_smallFont);
    if (m_headerFont != -1) DeleteFontToHandle(m_headerFont);
}

// メニュー選択と決定待ちを初期化し、背景・フォントを準備する。
void TitleScene::Enter()
{
    m_selected = 0;
    m_phase = 0.0f;
    m_inputLockTimer = 0.2f;
    m_showHelp = m_actionRequested = false;
    m_waitDecideRelease = true;
    if (m_background == -1)
    {
        m_background = LoadGraph("assets/images/title_bg.png");
        if (m_background == -1)
            printfDx("Could not load assets/images/title_bg.png\n");
    }
    if (m_menuFont == -1)
        m_menuFont = CreateFontToHandle("Arial", 26, 7, DX_FONTTYPE_ANTIALIASING_4X4);
    if (m_smallFont == -1)
        m_smallFont = CreateFontToHandle("Arial", 18, 4, DX_FONTTYPE_ANTIALIASING_4X4);
    if (m_headerFont == -1)
        m_headerFont = CreateFontToHandle("Arial", 32, 8, DX_FONTTYPE_ANTIALIASING_4X4);
}

// 人数選択をSceneContextへ保存し、ゲーム開始・説明表示・終了を切り替える。
void TitleScene::Update(float dt, const Input& input)
{
    if (!m_mgr || m_actionRequested || !std::isfinite(dt) || dt <= 0.0f) return;
    m_phase = std::fmod(m_phase + dt * 4.0f, 6.2831853f);
    m_inputLockTimer = m_inputLockTimer > dt ? m_inputLockTimer - dt : 0.0f;
    if (!input.Pressed(Action::Decide)) m_waitDecideRelease = false;
    if (input.Triggered(Action::ToggleFps))
        m_mgr->Context().showFps = !m_mgr->Context().showFps;
    if (input.Triggered(Action::ForceLow))
        m_mgr->Context().quality = m_mgr->Context().quality == QualityLevel::High
            ? QualityLevel::Low : QualityLevel::High;
    if (m_inputLockTimer > 0.0f) return;

    const bool decide = !m_waitDecideRelease && input.Triggered(Action::Decide);
    if (m_showHelp)
    {
        if (decide || input.Triggered(Action::Back))
        {
            m_showHelp = false;
            m_waitDecideRelease = true;
            Audio::Instance().PlaySe("back");
        }
        return;
    }
    if (input.Triggered(Action::Up)) m_selected = (m_selected + 3) % 4;
    else if (input.Triggered(Action::Down)) m_selected = (m_selected + 1) % 4;
    if (!decide) return;
    Audio::Instance().PlaySe("decide");
    if (m_selected <= 1)
    {
        m_actionRequested = true;
        m_mgr->Context().playerCount = m_selected + 1;
        m_mgr->RequestChange(SceneType::Game);
    }
    else if (m_selected == 2)
    {
        m_showHelp = true;
        m_waitDecideRelease = true;
    }
    else
    {
        // Appの既存終了経路で、音声とDxLibを後片付けして終了する。
        m_actionRequested = true;
        m_mgr->Context().quitRequested = true;
    }
}

// フォントが使えない場合も含め、文字幅から中央Xを求めて描く。
void TitleScene::DrawCentered_(const char* text, int y, int color, int font) const
{
    const int length = static_cast<int>(std::strlen(text));
    const int width = font != -1 ? GetDrawStringWidthToHandle(text, length, font)
        : GetDrawStringWidth(text, length);
    const int x = (SCREEN_W - width) / 2;
    if (font != -1) DrawStringToHandle(x, y, text, color, font);
    else DrawString(x, y, text, color);
}

// 1人/2人の操作と敵の倒し方を同じパネルへ表示する。
void TitleScene::DrawHelp_() const
{
    int mode = DX_BLENDMODE_NOBLEND, param = 0;
    GetDrawBlendMode(&mode, &param);
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 220);
    DrawBox(0, 0, SCREEN_W, SCREEN_H, GetColor(4, 10, 15), TRUE);
    SetDrawBlendMode(mode, param);
    DrawBox(270, 132, 1010, 610, GetColor(12, 29, 36), TRUE);
    DrawBox(270, 132, 1010, 610, GetColor(229, 167, 69), FALSE);
    const int white = GetColor(235, 237, 225);
    const int gold = GetColor(255, 201, 96);
    DrawCentered_("HOW TO PLAY", 162, gold, m_headerFont);
    DrawCentered_("1 PLAYER: ARROWS / A,D   SHIFT   SPACE", 224, white, m_smallFont);
    DrawCentered_("2 PLAYERS", 264, gold, m_menuFont);
    DrawCentered_("P1: A / D   L-SHIFT: RUN   SPACE: JUMP", 306, white, m_smallFont);
    DrawCentered_("P2: LEFT / RIGHT   R-SHIFT: RUN   ENTER: JUMP", 340, white, m_smallFont);
    DrawCentered_("PAD 1 / 2: MOVE   BUTTON 1: JUMP   BUTTON 2: RUN", 378, white, m_smallFont);
    DrawCentered_("Hit platforms from below, then touch flipped enemies.", 424, gold, m_smallFont);
    DrawCentered_("Crabs need two hits. POW affects grounded enemies.", 456, white, m_smallFont);
    DrawCentered_("Scores and lives are separate. Defeat all enemies together!", 488, white, m_smallFont);
    DrawCentered_("ENTER / SPACE: BACK     ESC: QUIT", 558, gold, m_smallFont);
}

// 背景と4項目のメニューを描き、必要なら操作説明パネルを重ねる。
void TitleScene::Draw()
{
    if (m_background != -1)
        DrawExtendGraph(0, 0, SCREEN_W, SCREEN_H, m_background, FALSE);
    else
    {
        DrawBox(0, 0, SCREEN_W, SCREEN_H, GetColor(8, 22, 30), TRUE);
        DrawCentered_("PIPE BROS", 180, GetColor(255, 190, 70), m_headerFont);
    }
    const int gold = GetColor(255, 197, 91);
    const int white = GetColor(239, 238, 218);
    char score[64];
    std::snprintf(score, sizeof(score), "HI-SCORE   %06.0f", m_mgr ? m_mgr->Context().hiScore : 0.0f);
    DrawCentered_(score, 342, gold, m_smallFont);

    int mode = DX_BLENDMODE_NOBLEND, param = 0;
    GetDrawBlendMode(&mode, &param);
    const char* labels[] = { "1 PLAYER", "2 PLAYERS", "HOW TO PLAY", "EXIT" };
    for (int i = 0; i < 4; ++i)
    {
        const int y = 376 + i * 58;
        const bool selected = i == m_selected;
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, selected ? 245 : 215);
        DrawBox(450, y, 830, y + 50, selected ? GetColor(225, 147, 43) : GetColor(12, 31, 38), TRUE);
        SetDrawBlendMode(mode, param);
        const int pulse = 180 + static_cast<int>(35.0f * (0.5f + 0.5f * std::sin(m_phase)));
        DrawBox(450, y, 830, y + 50, selected ? GetColor(255, pulse, 78) : GetColor(93, 112, 104), FALSE);
        DrawCentered_(labels[i], y + 10, selected ? GetColor(39, 29, 16) : white, m_menuFont);
        if (selected)
        {
            if (m_menuFont != -1) DrawStringToHandle(470, y + 10, ">", GetColor(39, 29, 16), m_menuFont);
            else DrawString(470, y + 10, ">", GetColor(39, 29, 16));
        }
    }
    DrawCentered_("UP / DOWN: SELECT     ENTER / SPACE: DECIDE     ESC: QUIT", 646, white, m_smallFont);
    if (m_showHelp) DrawHelp_();
}
