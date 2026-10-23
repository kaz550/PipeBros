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

    COUNT
};

class Input
{
public:
    void Update();

    bool Pressed(Action a) const;
    bool Triggered(Action a) const;
    bool Released(Action a) const;

    float GetMoveX() const { return m_moveX; }
    float GetMoveY() const { return m_moveY; }

private:
    std::array<int, (int)Action::COUNT> m_prev{};
    std::array<int, (int)Action::COUNT> m_curr{};
    float m_moveX = 0.0f;
    float m_moveY = 0.0f;

    int GetActionDown_(Action a, int pad) const;

    int GetPadState_() const;
    static int IsKeyDown_(int key);
    static int IsBitOn_(int state, int bit);
};
