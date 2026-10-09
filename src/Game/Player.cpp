#include "Player.h"
#include "../Input/Input.h"
#include "DxLib.h"

Player::Player()
{
}

void Player::Reset(float x, float groundY)
{
    m_x = x;
    m_groundY = groundY;
    m_y = m_groundY - m_height;
    m_vy = 0.0f;

    m_jumpHoldTimer = 0.0f;
    m_isGrounded = true;
    m_isJumpHolding = false;
    m_prevJumpKey = false;
}

void Player::Update(float dt, const Input& input)
{
    HandleJump_(dt);
    ApplyGravity_(dt);
    UpdatePosition_(dt);
    ClampToGround_();
}

void Player::Draw() const
{
    const int left = static_cast<int>(m_x);
    const int top = static_cast<int>(m_y);
    const int right = static_cast<int>(m_x + m_width);
    const int bottom = static_cast<int>(m_y + m_height);

    DrawBox(left, top, right, bottom, GetColor(255, 80, 80), TRUE);
    DrawBox(left + 8, top + 8, right - 8, top + 18, GetColor(255, 220, 220), TRUE);
}

Aabb Player::GetAabb() const
{
    Aabb aabb;

    // Œ©‚½–Ú‚æ‚è­‚µ‚¾‚¯¬‚³‚­‚µ‚ÄA—•ss‚ÈÚGŠ´‚ðŒ¸‚ç‚·
    const float marginX = 4.0f;
    const float marginY = 4.0f;

    aabb.l = m_x + marginX;
    aabb.t = m_y + marginY;
    aabb.r = m_x + m_width - marginX;
    aabb.b = m_y + m_height - marginY;

    return aabb;
}

void Player::HandleJump_(float dt)
{
    const bool jumpKey = CheckHitKey(KEY_INPUT_SPACE) != 0;
    const bool jumpPressed = jumpKey && !m_prevJumpKey;
    const bool jumpReleased = !jumpKey && m_prevJumpKey;

    if (m_isGrounded && jumpPressed)
    {
        m_vy = m_jumpVelocity;
        m_isGrounded = false;
        m_isJumpHolding = true;
        m_jumpHoldTimer = 0.0f;
    }

    if (m_isJumpHolding)
    {
        m_jumpHoldTimer += dt;

        if (!jumpKey || m_jumpHoldTimer >= m_maxJumpHoldTime || m_vy >= 0.0f)
        {
            m_isJumpHolding = false;
        }
    }

    if (jumpReleased && m_vy < 0.0f)
    {
        m_vy *= m_lowJumpRate;
        m_isJumpHolding = false;
    }

    m_prevJumpKey = jumpKey;
}

void Player::ApplyGravity_(float dt)
{
    if (m_isGrounded)
    {
        return;
    }

    float gravityRate = 1.0f;

    if (m_isJumpHolding && m_vy < 0.0f)
    {
        gravityRate = m_jumpHoldGravityRate;
    }

    m_vy += m_gravity * gravityRate * dt;
}

void Player::UpdatePosition_(float dt)
{
    m_y += m_vy * dt;
}

void Player::ClampToGround_()
{
    const float footY = m_y + m_height;

    if (footY >= m_groundY)
    {
        m_y = m_groundY - m_height;
        m_vy = 0.0f;
        m_isGrounded = true;
        m_isJumpHolding = false;
        m_jumpHoldTimer = 0.0f;
    }
    else
    {
        m_isGrounded = false;
    }
}