#pragma once
#include "Aabb.h"

class Input;

class Player
{
public:
    Player();

    void Reset(float x, float groundY);
    void Update(float dt, const Input& input);
    void Draw() const;

    Aabb GetAabb() const;

    float GetX() const { return m_x; }
    float GetY() const { return m_y; }
    float GetWidth() const { return m_width; }
    float GetHeight() const { return m_height; }
    float GetVelocityY() const { return m_vy; }
    bool IsGrounded() const { return m_isGrounded; }
    bool IsJumpHolding() const { return m_isJumpHolding; }

private:
    void HandleJump_(float dt);
    void ApplyGravity_(float dt);
    void UpdatePosition_(float dt);
    void ClampToGround_();

private:
    float m_x = 0.0f;
    float m_y = 0.0f;
    float m_vy = 0.0f;

    float m_width = 40.0f;
    float m_height = 80.0f;

    float m_gravity = 2200.0f;
    float m_jumpVelocity = -820.0f;

    float m_lowJumpRate = 0.45f;
    float m_jumpHoldGravityRate = 0.45f;
    float m_maxJumpHoldTime = 0.18f;
    float m_jumpHoldTimer = 0.0f;

    float m_groundY = 620.0f;

    bool m_isGrounded = false;
    bool m_isJumpHolding = false;
    bool m_prevJumpKey = false;
};