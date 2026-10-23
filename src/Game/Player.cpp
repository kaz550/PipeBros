#include "Player.h"
#include "../Input/Input.h"
#include "../Core/Config.h"
#include "DxLib.h"

#include <cmath>

Player::Player()
{
}

Player::~Player()
{
    if (m_spriteSheet != -1)
    {
        if (DxLib_IsInit() != 0)
        {
            DeleteGraph(m_spriteSheet);
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

    const char* path = m_playerIndex == 1
        ? "assets/images/pipe_bros_2p.png" : "assets/images/pipe_bros.png";
    m_spriteSheet = LoadGraph(path);

    if (m_spriteSheet == -1)
    {
        m_error = std::string("Failed to load: ") + path;
        printfDx("%s\n", m_error.c_str());
        return false;
    }

    int width = 0;
    int height = 0;

    const bool valid =
        GetGraphSize(m_spriteSheet, &width, &height) != -1
        && width == 1402
        && height == 1122;

    if (!valid)
    {
        m_error = std::string(path) + " must be 1402 x 1122 pixels.";
        printfDx("%s\n", m_error.c_str());

        DeleteGraph(m_spriteSheet);
        m_spriteSheet = -1;
        return false;
    }

    return true;
}

void Player::Reset(const StageData& stage, int playerIndex)
{
    if (playerIndex != m_playerIndex && m_spriteSheet != -1)
    {
        if (DxLib_IsInit() != 0) DeleteGraph(m_spriteSheet);
        m_spriteSheet = -1;
    }
    m_playerIndex = playerIndex;
    m_stage = &stage;
    m_error.clear();
    LoadSprite_();
    m_groundY = stage.IsLoaded() ? stage.GetGroundY() : 620.0f;
    m_x = stage.IsLoaded() ? stage.GetPlayerX() : 220.0f;
    if (playerIndex == 1) m_x = SCREEN_W - m_x - m_width;
    m_y = (stage.IsLoaded() ? stage.GetPlayerFootY() : m_groundY) - m_height;
    m_respawnX = stage.IsLoaded() ? stage.GetPlayerRespawnCenterX() - m_width * 0.5f
        : (SCREEN_W - m_width) * 0.5f;
    m_respawnFootY = stage.IsLoaded() ? stage.GetPlayerRespawnFootY() : m_groundY;
    m_lastBumpedObjectId = -1;

    m_vx = 0.0f;
    m_vy = 0.0f;

    m_isGrounded = true;
    m_isJumpHolding = false;
    m_jumpHoldTimer = 0.0f;

    m_isRunning = false;
    m_facingLeft = m_playerIndex == 1;

    m_animation = Animation::Idle;
    m_animationTimer = 0.0f;
    m_frameIndex = 0;

    m_jumpPoseTimer = 0.0f;
    m_landingTimer = 0.0f;
}

void Player::Respawn()
{
    // 画像やTMXを再読み込みせず、現在のPOWの状態に応じて復帰先を選ぶ。
    if (m_stage && m_stage->IsLoaded() && m_stage->GetPowUses() > 0)
    {
        const StageSolid& pow = m_stage->GetPow();
        m_x = pow.x + (pow.width - m_width) * 0.5f;
        m_y = pow.y - m_height;
    }
    else
    {
        // POWが使用済み・未配置なら、従来の下段中央ポイントを使う。
        m_x = m_respawnX;
        m_y = m_respawnFootY - m_height;
    }
    m_vx = m_vy = 0.0f;
    m_isGrounded = false;
    m_isJumpHolding = false;
    m_jumpHoldTimer = 0.0f;
    m_isRunning = false;
    m_facingLeft = m_playerIndex == 1;
    m_animation = Animation::Idle;
    m_animationTimer = 0.0f;
    m_frameIndex = 0;
    m_jumpPoseTimer = m_landingTimer = 0.0f;
    m_lastBumpedObjectId = -1;
}

void Player::Update(float dt, const Input& input)
{
    if (dt <= 0.0f)
    {
        return;
    }


    m_lastBumpedObjectId = -1;

    if (m_jumpPoseTimer > 0.0f)
    {
        m_jumpPoseTimer -= dt;
    }

    if (m_landingTimer > 0.0f)
    {
        m_landingTimer -= dt;
    }

    const bool wasGrounded = m_isGrounded;

    HandleMove_(input);
    HandleJump_(dt, input);
    ApplyGravity_(dt);
    UpdatePosition_(dt);
    ClampToGround_();

    if (!wasGrounded && m_isGrounded)
    {
        m_landingTimer = 0.08f;
        m_jumpPoseTimer = 0.0f;
    }

    UpdateAnimation_(dt);
}

void Player::HandleMove_(const Input& input)
{
    const bool left =
        input.Pressed(Action::Left);

    const bool right =
        input.Pressed(Action::Right);

    const bool shift =
        input.Pressed(Action::Run);

    int direction = 0;

    if (left)
    {
        --direction;
    }

    if (right)
    {
        ++direction;
    }

    m_isRunning = shift && direction != 0;

    const float speed =
        m_isRunning ? m_runSpeed : m_walkSpeed;

    m_vx = direction * speed;

    if (direction < 0)
    {
        m_facingLeft = true;
    }
    else if (direction > 0)
    {
        m_facingLeft = false;
    }
}

void Player::HandleJump_(float dt, const Input& input)
{
    const bool jumpKey =
        input.Pressed(Action::Jump);

    const bool pressed =
        input.Triggered(Action::Jump);

    const bool released =
        input.Released(Action::Jump);

    if (m_isGrounded && pressed)
    {
        m_vy = m_jumpVelocity;

        m_isGrounded = false;
        m_isJumpHolding = true;
        m_jumpHoldTimer = 0.0f;

        m_jumpPoseTimer = 0.04f;
        m_landingTimer = 0.0f;
    }

    if (m_isJumpHolding)
    {
        m_jumpHoldTimer += dt;

        if (!jumpKey ||
            m_jumpHoldTimer >= m_maxJumpHoldTime ||
            m_vy >= 0.0f)
        {
            m_isJumpHolding = false;
        }
    }

    if (released && m_vy < 0.0f)
    {
        m_vy *= m_lowJumpRate;
        m_isJumpHolding = false;
    }
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
    if (m_stage && m_stage->IsLoaded())
    {
        const float previousVelocityY = m_vy;
        m_stage->Move(m_x, m_y, m_vx, m_vy,
            m_width, m_height, dt, static_cast<float>(SCREEN_W),
            m_isGrounded, m_lastBumpedObjectId);
        if (m_isGrounded || (previousVelocityY < 0.0f && m_vy == 0.0f))
        {
            m_isJumpHolding = false;
            m_jumpHoldTimer = 0.0f;
            m_jumpPoseTimer = 0.0f;
        }
        return;
    }
    m_x += m_vx * dt;
    m_y += m_vy * dt;

    float centerX = m_x + m_width * 0.5f;

    while (centerX < 0.0f)
    {
        m_x += static_cast<float>(SCREEN_W);
        centerX += static_cast<float>(SCREEN_W);
    }

    while (centerX >= static_cast<float>(SCREEN_W))
    {
        m_x -= static_cast<float>(SCREEN_W);
        centerX -= static_cast<float>(SCREEN_W);
    }
}

void Player::ClampToGround_()
{
    if (m_stage && m_stage->IsLoaded())
    {
        return; // 接地状態はTMXの足場判定で確定している。
    }
    const float footY = m_y + m_height;

    if (m_vy >= 0.0f && footY >= m_groundY)
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

void Player::ChangeAnimation_(Animation animation)
{
    if (m_animation == animation)
    {
        return;
    }

    m_animation = animation;
    m_animationTimer = 0.0f;
}

void Player::UpdateAnimation_(float dt)
{
    if (!m_isGrounded)
    {
        ChangeAnimation_(Animation::Jump);

        if (m_jumpPoseTimer > 0.0f)
        {
            m_frameIndex = 8;
        }
        else if (m_vy < -120.0f)
        {
            m_frameIndex = 9;
        }
        else if (m_vy < 120.0f)
        {
            m_frameIndex = 10;
        }
        else
        {
            m_frameIndex = 11;
        }

        return;
    }

    if (m_landingTimer > 0.0f)
    {
        ChangeAnimation_(Animation::Jump);
        m_frameIndex = 12;
        return;
    }

    const bool moving = std::fabs(m_vx) > 0.01f;

    const Animation next =
        !moving ? Animation::Idle :
        m_isRunning ? Animation::Run :
        Animation::Walk;

    ChangeAnimation_(next);

    int firstFrame = 0;
    int frameCount = 2;
    float frameDuration = 0.40f;

    if (m_animation == Animation::Walk)
    {
        firstFrame = 2;
        frameCount = 4;
        frameDuration = 0.12f;
    }
    else if (m_animation == Animation::Run)
    {
        firstFrame = 5;
        frameCount = 4;
        frameDuration = 0.075f;
    }

    const float cycleDuration =
        frameDuration * frameCount;

    m_animationTimer =
        std::fmod(m_animationTimer + dt, cycleDuration);

    const int localFrame =
        static_cast<int>(m_animationTimer / frameDuration);

    // 歩き・走りは3枚を「1 → 2 → 3 → 2」で再生する。
    // frameCountの4は画像枚数ではなく再生ステップ数。
    m_frameIndex = firstFrame
        + (moving && localFrame == 3 ? 1 : localFrame);
}

void Player::Draw(int alpha) const
{
    if (m_spriteSheet == -1 || alpha <= 0)
    {
        return;
    }

    // 最新の1402×1122ピクセルのシート用。
    //
    // x, y, 幅, 高さ, 基準点X, 基準点Y
    //
    // 基準点は切り出し画像の左上からの相対座標。
    // 基準点Yは画像外に置くこともできる。
    // 走りの空中姿勢では靴底より下に置き、
    // キャラクターを地面から少し浮かせる。
    static const SpriteFrame frames[] =
    {
        // アイドル：0～1
        {  68,  50, 189, 241,  97.0f, 239.0f },
        { 336,  50, 188, 241,  97.0f, 239.0f },

        // 歩き：2～4（3コマ）
        {  54, 308, 209, 259, 111.0f, 257.0f },
        { 339, 308, 187, 259,  96.0f, 257.0f },
        { 598, 308, 205, 259, 107.0f, 257.0f },

        // 走り：5～7（3コマ）
        {  29, 577, 250, 260, 136.0f, 258.0f },
        { 326, 577, 215, 260, 109.0f, 258.0f },
        { 562, 577, 254, 260, 143.0f, 258.0f },

        // ジャンプ：8～12（5コマ）
        {  59, 882, 201, 213, 101.0f, 211.0f },
        { 321, 837, 226, 258, 113.0f, 256.0f },
        { 585, 837, 223, 229, 112.0f, 227.0f },
        { 842, 837, 236, 268, 118.0f, 266.0f },
        {1148, 883, 205, 212, 103.0f, 210.0f },
    };

    const int frameCount =
        static_cast<int>(sizeof(frames) / sizeof(frames[0]));

    if (m_frameIndex < 0 || m_frameIndex >= frameCount)
    {
        return;
    }

    const SpriteFrame& frame = frames[m_frameIndex];

    const float anchorX = m_x + m_width * 0.5f;
    const float anchorY = m_y + m_height;

    // DxLibへ渡す回転中心と、位置計算に使う中心を揃える
    const int sourceCenterX = frame.width / 2;
    const int sourceCenterY = frame.height / 2;

    const float centerOffsetX =
        (sourceCenterX - frame.pivotX)
        * m_spriteScale;

    const float centerOffsetY =
        (sourceCenterY - frame.pivotY)
        * m_spriteScale;

    const auto drawAt = [&](float x)
    {
        const float drawCenterX =
            x + (m_facingLeft
                ? -centerOffsetX
                : centerOffsetX);

        const float drawCenterY =
            anchorY + centerOffsetY;

        DrawRectRotaGraph3(
            static_cast<int>(drawCenterX),
            static_cast<int>(drawCenterY),
            frame.x,
            frame.y,
            frame.width,
            frame.height,
            sourceCenterX,
            sourceCenterY,
            m_spriteScale,
            m_spriteScale,
            0.0,
            m_spriteSheet,
            TRUE,
            m_facingLeft ? TRUE : FALSE
        );
    };

    // プレイヤーだけに適用し、画面端の複製も同じ透明度で描く。
    int previousBlendMode = DX_BLENDMODE_NOBLEND;
    int previousBlendParam = 0;
    GetDrawBlendMode(&previousBlendMode, &previousBlendParam);
    if (alpha > 255) alpha = 255;
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
    drawAt(anchorX);

    const float extentX =
        (frame.pivotX > frame.width - frame.pivotX
            ? frame.pivotX
            : frame.width - frame.pivotX)
        * m_spriteScale;

    if (anchorX - extentX < 0.0f)
    {
        drawAt(anchorX + SCREEN_W);
    }

    if (anchorX + extentX > SCREEN_W)
    {
        drawAt(anchorX - SCREEN_W);
    }
    // 続いて描く敵やHUDに透明度が残らないよう元の設定に戻す。
    SetDrawBlendMode(previousBlendMode, previousBlendParam);
}

Aabb Player::GetAabb() const
{
    Aabb aabb;

    const float marginX = 4.0f;
    const float marginY = 4.0f;

    aabb.l = m_x + marginX;
    aabb.t = m_y + marginY;
    aabb.r = m_x + m_width - marginX;
    aabb.b = m_y + m_height - marginY;

    return aabb;
}
