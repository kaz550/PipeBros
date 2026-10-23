// 土管兄弟 ソース解説
// 背景画像、人数選択、操作説明、終了メニューを表示する。
// 人数はSceneContextへ保存する。メニュー決定の長押しを次画面へ持ち越さない。

#pragma once
#include "IScene.h"

class SceneManager;
class Input;

class TitleScene : public IScene
{
public:
    explicit TitleScene(SceneManager* mgr) : m_mgr(mgr) {}
    ~TitleScene() override;
    void Enter() override;
    void Update(float dt, const Input& input) override;
    void Draw() override;

private:
    void DrawCentered_(const char* text, int y, int color, int font) const;
    void DrawHelp_() const;
    SceneManager* m_mgr = nullptr;
    int m_background = -1;
    int m_menuFont = -1, m_smallFont = -1, m_headerFont = -1;
    int m_selected = 0;
    float m_phase = 0.0f, m_inputLockTimer = 0.2f;
    bool m_showHelp = false;
    bool m_waitDecideRelease = true;
    bool m_actionRequested = false;
};
