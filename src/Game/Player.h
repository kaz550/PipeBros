#pragma once

#include "Aabb.h"

class Input;

class Player
{
public:
    Player();
    ~Player();

    Player(const Player&) = delete;
    Player& operator=(const Player&) = delete;

    void Reset(float x, float groundY);
    void Update(float dt, const Input& input);
    void Draw() const;

    Aabb GetAabb() const;

    float GetX() const { return m_x; }
    float GetY() const { return m_y; }
    float GetWidth() const { return m_width; }
    float GetHeight() const { return m_height; }

    float GetVelocityX() const { return m_vx; }
    float GetVelocityY() const { return m_vy; }

    bool IsGrounded() const { return m_isGrounded; }
    bool IsJumpHolding() const { return m_isJumpHolding; }

private:
    struct SpriteFrame
    {
        // スプライトシート内の切り出し範囲
        int x;
        int y;
        int width;
        int height;

        // 切り出し範囲の左上から見た基準点
        float pivotX;
        float pivotY;
    };

    enum class Animation
    {
        Idle,
        Walk,
        Run,
        Jump
    };

    bool LoadSprite_();

    void HandleMove_();
    void HandleJump_(float dt);
    void ApplyGravity_(float dt);
    void UpdatePosition_(float dt);
    void ClampToGround_();

    void UpdateAnimation_(float dt);
    void ChangeAnimation_(Animation animation);

private:
    // 当たり判定の左上位置
    float m_x = 0.0f;
    float m_y = 0.0f;

    float m_vx = 0.0f;
    float m_vy = 0.0f;

    // 画像の大きさとは独立した当たり判定
    float m_width = 40.0f;
    float m_height = 80.0f;

    float m_walkSpeed = 220.0f;
    float m_runSpeed = 360.0f;

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

    bool m_isRunning = false;
    bool m_facingLeft = false;

    int m_spriteSheet = -1;
    int m_frameIndex = 0;

    // アイドルの高さ約238pxを約80pxで表示
    float m_spriteScale = 0.34f;

    Animation m_animation = Animation::Idle;
    float m_animationTimer = 0.0f;

    float m_jumpPoseTimer = 0.0f;
    float m_landingTimer = 0.0f;
};