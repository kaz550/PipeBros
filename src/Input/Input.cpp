#include "Input.h"
#include "DxLib.h"

int Input::IsKeyDown_(int key)
{
    return (CheckHitKey(key) != 0) ? 1 : 0;
}

int Input::IsBitOn_(int state, int bit)
{
    return (state & bit) ? 1 : 0;
}

int Input::GetPadState_() const
{
    return GetJoypadInputState(DX_INPUT_KEY_PAD1);
}

int Input::GetActionDown_(Action a) const
{
    const int pad = GetPadState_();

    const int padLeft = IsBitOn_(pad, PAD_INPUT_LEFT);
    const int padRight = IsBitOn_(pad, PAD_INPUT_RIGHT);
    const int padUp = IsBitOn_(pad, PAD_INPUT_UP);
    const int padDown = IsBitOn_(pad, PAD_INPUT_DOWN);

    const int padA = IsBitOn_(pad, PAD_INPUT_1);
    const int padB = IsBitOn_(pad, PAD_INPUT_2);

    switch (a)
    {
    case Action::Left:
        return IsKeyDown_(KEY_INPUT_LEFT) || IsKeyDown_(KEY_INPUT_A) || padLeft;
    case Action::Right:
        return IsKeyDown_(KEY_INPUT_RIGHT) || IsKeyDown_(KEY_INPUT_D) || padRight;
    case Action::Up:
        return IsKeyDown_(KEY_INPUT_UP) || IsKeyDown_(KEY_INPUT_W) || padUp;
    case Action::Down:
        return IsKeyDown_(KEY_INPUT_DOWN) || IsKeyDown_(KEY_INPUT_S) || padDown;
    case Action::Decide:
        return IsKeyDown_(KEY_INPUT_RETURN) || IsKeyDown_(KEY_INPUT_SPACE) || padA;
    case Action::Back:
        return IsKeyDown_(KEY_INPUT_ESCAPE) || padB;
    case Action::Quit:
        return IsKeyDown_(KEY_INPUT_ESCAPE);

    case Action::ToggleFps:
        return IsKeyDown_(KEY_INPUT_F1);
    case Action::ForceLow:
        return IsKeyDown_(KEY_INPUT_F2);

    default:
        return 0;
    }
}

void Input::Update()
{
    m_prev = m_curr;

    for (int i = 0; i < (int)Action::COUNT; ++i)
        m_curr[i] = GetActionDown_((Action)i);

    m_moveX = 0.0f;
    if (Pressed(Action::Left))  m_moveX -= 1.0f;
    if (Pressed(Action::Right)) m_moveX += 1.0f;

    m_moveY = 0.0f;
    if (Pressed(Action::Up))    m_moveY -= 1.0f;
    if (Pressed(Action::Down))  m_moveY += 1.0f;
}

bool Input::Pressed(Action a) const { return m_curr[(int)a] != 0; }
bool Input::Triggered(Action a) const { return (m_prev[(int)a] == 0) && (m_curr[(int)a] != 0); }
bool Input::Released(Action a) const { return (m_prev[(int)a] != 0) && (m_curr[(int)a] == 0); }