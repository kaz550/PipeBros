#pragma once
#include "Aabb.h"
#include "StageData.h"


class Input;

class Player
{
public:
    Player();
    ~Player();
    Player(const Player&) = delete;
    Player& operator=(const Player&) = delete;

    void Reset(const StageData& stage, int playerIndex);
    void Update(float dt, const Input& input);
    // alpha: 0なら非表示、255なら通常表示。ステージには適用しない。
    void Draw(int alpha = 255) const;
    void Respawn();
    Aabb GetAabb() const;
    // 敵用より少し広い、プレイヤー同士の本体矩形。
    Aabb GetContactAabb() const { return { m_x, m_y, m_x + m_width, m_y + m_height }; }
    static void ResolveContact(Player& first, Player& second,
        const Aabb& beforeFirst, const Aabb& beforeSecond);
    const std::string& GetError() const { return m_error; }

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

    // 接触補正にもステージ判定を使い、壁・POWへ押し込まない。
    float MoveByContact_(float dx, float dy);
    void LaunchByPlayer_(int direction);
    static constexpr float CONTACT_PUSH_SPEED = 280.0f;
    static constexpr float CONTACT_LAUNCH_SPEED = -460.0f;
    static constexpr float CONTACT_LOCK_DURATION = 0.20f;
    float m_contactLockTimer = 0.0f;
    float m_contactVelocityX = 0.0f;
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
    const StageData* m_stage = nullptr;
    int m_playerIndex = 0;
    std::string m_error;
    int m_lastBumpedObjectId = -1;

    int m_spriteSheet = -1, m_frameIndex = 0;
    float m_spriteScale = 0.34f;
    Animation m_animation = Animation::Idle;
    float m_animationTimer = 0.0f;
    float m_jumpPoseTimer = 0.0f, m_landingTimer = 0.0f;
};
