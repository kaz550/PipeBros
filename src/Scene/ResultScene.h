// 土管兄弟 ソース解説
// ゲームオーバー時の個人得点と共通成績を表示し、ハイスコア保存とリトライを扱う。
// 通常クリアはGameScene内で次の面へ進むため、この画面を経由しない。

#pragma once
#include "IScene.h"
#include "../Game/StageRenderer.h"

class SceneManager;

class ResultScene : public IScene
{
public:
    explicit ResultScene(SceneManager* mgr) : m_mgr(mgr) {}
    ~ResultScene() override;
    void Enter() override;
    void Update(float dt, const class Input& input) override;
    void Draw() override;

private:
    void DrawCentered_(const char* text, int y, int color, int font) const;
    SceneManager* m_mgr = nullptr;
    StageRenderer m_background;
    int m_selected = 0;
    int m_menuFont = -1;
    int m_smallFont = -1;
    int m_headerFont = -1;
    bool m_actionRequested = false;
    bool m_isNew = false;
    bool m_saveFailed = false;
    bool m_waitDecideRelease = true;
    float m_inputLockTimer = 0.25f;
};
