#include "Player.h"
#include "../Input/Input.h"
#include "DxLib.h"

Player::Player()
{
}

Player::~Player() 
{
    if (m_spriteSheet != -1) 
    {
        if (DxLib_IsInit() != 0) 
        {
            // 後で追加する
        }
        m_spriteSheet = -1;
    }
    
}
bool Player::LoadSprite_()
{
    if (m_spriteSheet != -1)
    {
        return true;
    }
    m_spriteSheet = LoadGraph(
        "assets/images/pipe_bros.png"
    );
    if (m_spriteSheet == -1)
    {
        printfDx(
            "Failed to Load: assets/images/pipe_bros.png\n"
        );
        return false;
    }
    int imageWidth = 0;
    int imageHeight = 0;
    if (GetGraphSize(m_spriteSheet, &imageWidth, &imageHeight) == -1)
    {
        printfDx("Failed to get sprite sheet size\n");
        DeleteGraph(m_spriteSheet);
        m_spriteSheet = -1;
        return false;
    }

    if (imageWidth != 1536 || imageHeight != 1024)
    {
        printfDx("pipe_bros.png must be 1536 x 1024 pixels.\n");
        DeleteGraph(m_spriteSheet);
        m_spriteSheet = -1;
        return false;
    }

    return true;
}
void Player::Reset(float x, float groundY)
{
    LoadSprite_();

    m_x = x;
    m_groundY = groundY;
    m_y = m_groundY - m_height;
    m_vy = 0.0f;

    m_jumpHoldTimer = 0.0f;
    m_isGrounded = true;
    m_isJumpHolding = false;
    m_prevJumpKey = false;

    //スプライト表示テスト
    m_frameIndex = 0;
}

void Player::Update(float dt, const Input& input)
{
    (void)input;

    HandleJump_(dt);
    ApplyGravity_(dt);
    UpdatePosition_(dt);
    ClampToGround_();
}

void Player::Draw() const
{
    if (m_spriteSheet == -1)
    {
        return;
    }

    static const SpriteFrame frames[] =
    {
        { 82, 57, 150, 202, 75.0f, 200.0f},
        {322, 55, 145, 204, 72.5f, 202.0f},
    };

    constexpr int frameCount =
        static_cast<int>(
            sizeof(frames) / sizeof(frames[0])
            );
    if (m_frameIndex < 0 || m_frameIndex >= frameCount)
    {
        return;
    }
    const SpriteFrame& frame = frames[m_frameIndex];

    const float anchorX = m_x + m_width * 0.5f;
    const float anchorY = m_y + m_height;

    

    const float left = 
        anchorX - frame.pivotX * m_spriteScale;
    const float top =
        anchorY - frame.pivotY * m_spriteScale;
    const float right =
        left + frame.width * m_spriteScale;
    const float bottom =
        top + frame.height * m_spriteScale;

    DrawRectExtendGraph(
        static_cast<int>(left),
        static_cast<int>(top),
        static_cast<int>(right),
        static_cast<int>(bottom),
        frame.x,
        frame.y,
        frame.width,
        frame.height,
        m_spriteSheet,
        TRUE
    );
    //DrawBox(left, top, right, bottom, GetColor(255, 80, 80), TRUE);
    //DrawBox(left + 8, top + 8, right - 8, top + 18, GetColor(255, 220, 220), TRUE);
}

Aabb Player::GetAabb() const
{
    Aabb aabb;

    // 見た目より少しだけ小さくして、理不尽な接触感を減らす
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