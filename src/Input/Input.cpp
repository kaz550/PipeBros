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

int Input::GetActionDown_(Action a, int pad) const
{
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

    case Action::Jump:
        return IsKeyDown_(KEY_INPUT_SPACE) || padA;
    case Action::Run:
        return IsKeyDown_(KEY_INPUT_LSHIFT) || IsKeyDown_(KEY_INPUT_RSHIFT) || padB;
    case Action::Restart:
        return IsKeyDown_(KEY_INPUT_R);
    case Action::ToggleDebug:
        return IsKeyDown_(KEY_INPUT_F3);
    case Action::ToTitle:
        return IsKeyDown_(KEY_INPUT_T);

    case Action::ToggleStageSelect:
        return IsKeyDown_(KEY_INPUT_F4);

    default:
        return 0;
    }
}

void Input::Update()
{
    m_prev = m_curr;
    m_singlePrev = m_singleCurr;
    m_playerPrev = m_playerCurr;
    // KEY_PAD1は矢印キーも含むため使わない。物理パッドを個別に読む。
    const int pads[] = { GetJoypadInputState(DX_INPUT_PAD1), GetJoypadInputState(DX_INPUT_PAD2) };
    for (int i = 0; i < (int)Action::COUNT; ++i)
        m_curr[i] = GetActionDown_((Action)i, pads[0] | pads[1]);
    m_singleCurr = m_curr;
    m_singleCurr[(int)Action::Left] = IsKeyDown_(KEY_INPUT_LEFT) || IsKeyDown_(KEY_INPUT_A) || IsBitOn_(pads[0], PAD_INPUT_LEFT);
    m_singleCurr[(int)Action::Right] = IsKeyDown_(KEY_INPUT_RIGHT) || IsKeyDown_(KEY_INPUT_D) || IsBitOn_(pads[0], PAD_INPUT_RIGHT);
    m_singleCurr[(int)Action::Jump] = IsKeyDown_(KEY_INPUT_SPACE) || IsBitOn_(pads[0], PAD_INPUT_1);
    m_singleCurr[(int)Action::Run] = IsKeyDown_(KEY_INPUT_LSHIFT) || IsKeyDown_(KEY_INPUT_RSHIFT) || IsBitOn_(pads[0], PAD_INPUT_2);
    m_playerCurr[0] = m_playerCurr[1] = m_curr;
    m_playerCurr[0][(int)Action::Left] = IsKeyDown_(KEY_INPUT_A) || IsBitOn_(pads[0], PAD_INPUT_LEFT);
    m_playerCurr[0][(int)Action::Right] = IsKeyDown_(KEY_INPUT_D) || IsBitOn_(pads[0], PAD_INPUT_RIGHT);
    m_playerCurr[0][(int)Action::Jump] = IsKeyDown_(KEY_INPUT_SPACE) || IsBitOn_(pads[0], PAD_INPUT_1);
    m_playerCurr[0][(int)Action::Run] = IsKeyDown_(KEY_INPUT_LSHIFT) || IsBitOn_(pads[0], PAD_INPUT_2);
    m_playerCurr[1][(int)Action::Left] = IsKeyDown_(KEY_INPUT_LEFT) || IsBitOn_(pads[1], PAD_INPUT_LEFT);
    m_playerCurr[1][(int)Action::Right] = IsKeyDown_(KEY_INPUT_RIGHT) || IsBitOn_(pads[1], PAD_INPUT_RIGHT);
    m_playerCurr[1][(int)Action::Jump] = IsKeyDown_(KEY_INPUT_RETURN) || IsBitOn_(pads[1], PAD_INPUT_1);
    m_playerCurr[1][(int)Action::Run] = IsKeyDown_(KEY_INPUT_RSHIFT) || IsBitOn_(pads[1], PAD_INPUT_2);
    SetMoveAxes_();
}

Input Input::ForPlayer(int playerIndex, int playerCount) const
{
    Input result = *this;
    if (playerCount <= 1)
    {
        result.m_prev = m_singlePrev;
        result.m_curr = m_singleCurr;
    }
    else
    {
        const int index = playerIndex == 1 ? 1 : 0;
        result.m_prev = m_playerPrev[index];
        result.m_curr = m_playerCurr[index];
    }
    result.SetMoveAxes_();
    return result;
}

void Input::SetMoveAxes_()
{
    m_moveX = (Pressed(Action::Right) ? 1.0f : 0.0f) - (Pressed(Action::Left) ? 1.0f : 0.0f);
    m_moveY = (Pressed(Action::Down) ? 1.0f : 0.0f) - (Pressed(Action::Up) ? 1.0f : 0.0f);
}

bool Input::Pressed(Action a) const { return m_curr[(int)a] != 0; }
bool Input::Triggered(Action a) const { return (m_prev[(int)a] == 0) && (m_curr[(int)a] != 0); }
bool Input::Released(Action a) const { return (m_prev[(int)a] != 0) && (m_curr[(int)a] == 0); }
