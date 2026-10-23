#pragma once
#include <array>

enum class Action
{
    Left,
    Right,
    Up,
    Down,
    Decide,
    Back,
    Quit,

    ToggleFps, // F1
    ForceLow,  // F2

    Jump,        // SPACE / PAD 1
    Run,         // SHIFT / PAD 2
    Restart,     // R
    ToggleDebug, // F3
    ToTitle,     // T

    ToggleStageSelect, // F4（F3のデバッグ表示中）

    COUNT
};

class Input
{
public:
    void Update();
    // ゲーム用の入力を分離。1人時は従来のWASD・矢印を両方利用できる。
    Input ForPlayer(int playerIndex, int playerCount) const;

    bool Pressed(Action a) const;
    bool Triggered(Action a) const;
    bool Released(Action a) const;

    float GetMoveX() const { return m_moveX; }
    float GetMoveY() const { return m_moveY; }

private:
    std::array<int, (int)Action::COUNT> m_prev{};
    std::array<int, (int)Action::COUNT> m_curr{};
    std::array<int, (int)Action::COUNT> m_singlePrev{}, m_singleCurr{};
    std::array<std::array<int, (int)Action::COUNT>, 2> m_playerPrev{}, m_playerCurr{};
    float m_moveX = 0.0f;
    float m_moveY = 0.0f;

    int GetActionDown_(Action a, int pad) const;

    void SetMoveAxes_();
    static int IsKeyDown_(int key);
    static int IsBitOn_(int state, int bit);
};
