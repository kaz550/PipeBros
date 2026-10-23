#pragma once
#include "Aabb.h"
#include "StageData.h"
#include "StageRenderer.h"

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
    // alpha: 0なら非表示、255なら通常表示。ステージには適用しない。
    void Draw(int alpha = 255) const;
    void Respawn();
    // GameSceneでPOWの衝撃を敵へ適用した後に呼ぶ。
    void UsePow();
    bool IsStageDebugVisible() const { return m_showStageDebug; }
    void SetStageDebugVisible(bool visible) { m_showStageDebug = visible; }
    void UpdateVisualEffects(float dt);
    Aabb GetAabb() const;
    const StageData& GetStageData() const { return m_stage; }

    float GetX() const { return m_x; }
    float GetY() const { return m_y; }
    float GetWidth() const { return m_width; }
    float GetHeight() const { return m_height; }
    float GetVelocityX() const { return m_vx; }
    float GetVelocityY() const { return m_vy; }
    bool IsGrounded() const { return m_isGrounded; }
    bool IsJumpHolding() const { return m_isJumpHolding; }
    // 今回のUpdateで下から叩いた足場のID。該当なしは-1。
    int GetLastBumpedObjectId() const { return m_lastBumpedObjectId; }

private:
    struct SpriteFrame
    {
        int x, y, width, height;
        float pivotX, pivotY;
    };
    enum class Animation { Idle, Walk, Run, Jump };

    bool LoadSprite_();
    void HandleMove_(const Input& input);
    void HandleJump_(float dt, const Input& input);
    void ApplyGravity_(float dt);
    void UpdatePosition_(float dt);
    void ClampToGround_();
    void UpdateAnimation_(float dt);
    void ChangeAnimation_(Animation animation);

    float m_x = 0.0f, m_y = 0.0f;
    float m_vx = 0.0f, m_vy = 0.0f;
    float m_width = 40.0f, m_height = 80.0f;
    float m_walkSpeed = 220.0f, m_runSpeed = 360.0f;
    float m_gravity = 2200.0f, m_jumpVelocity = -820.0f;
    float m_lowJumpRate = 0.45f;
    float m_jumpHoldGravityRate = 0.45f;
    float m_maxJumpHoldTime = 0.18f, m_jumpHoldTimer = 0.0f;
    float m_groundY = 620.0f;
    bool m_isGrounded = false, m_isJumpHolding = false;
    bool m_isRunning = false;
    bool m_facingLeft = false;

    float m_respawnX = 220.0f;
    float m_respawnFootY = 620.0f;
    StageData m_stage;
    StageRenderer m_stageRenderer;
    int m_lastBumpedObjectId = -1;
    bool m_showStageDebug = false;

    int m_spriteSheet = -1, m_frameIndex = 0;
    float m_spriteScale = 0.34f;
    Animation m_animation = Animation::Idle;
    float m_animationTimer = 0.0f;
    float m_jumpPoseTimer = 0.0f, m_landingTimer = 0.0f;
};
