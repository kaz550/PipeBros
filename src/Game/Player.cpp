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
    m_contactLockTimer = m_contactVelocityX = 0.0f;

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
    m_contactLockTimer = m_contactVelocityX = 0.0f;
}

void Player::Update(float dt, const Input& input)
{
    if (dt <= 0.0f)
    {
        return;
    }

    m_contactLockTimer = std::fmax(0.0f, m_contactLockTimer - dt);
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

float Player::MoveByContact_(float dx, float dy)
{
    const float oldX = m_x;
    if (m_stage && m_stage->IsLoaded())
    {
        float vx = dx, vy = dy;
        bool grounded = false;
        int ignoredBump = -1;
        m_stage->Move(m_x, m_y, vx, vy, m_width, m_height, 1.0f,
            static_cast<float>(SCREEN_W), grounded, ignoredBump);
        m_isGrounded = grounded;
        // 補正で触れた天井を、新しい床叩き・POW使用としては扱わない。
    }
    else
    {
        m_x += dx;
        m_y += dy;
        while (m_x + m_width * 0.5f < 0.0f) m_x += SCREEN_W;
        while (m_x + m_width * 0.5f >= SCREEN_W) m_x -= SCREEN_W;
        if (m_y + m_height > m_groundY) m_y = m_groundY - m_height;
    }
    float moved = m_x - oldX;
    moved -= std::round(moved / SCREEN_W) * SCREEN_W;
    return moved;
}

void Player::LaunchByPlayer_(int direction)
{
    m_contactVelocityX = direction * CONTACT_PUSH_SPEED;
    m_contactLockTimer = CONTACT_LOCK_DURATION;
    m_vx = m_contactVelocityX;
    m_vy = CONTACT_LAUNCH_SPEED;
    m_isGrounded = m_isJumpHolding = m_isRunning = false;
    m_jumpHoldTimer = m_jumpPoseTimer = m_landingTimer = 0.0f;
}

void Player::ResolveContact(Player& first, Player& second,
    const Aabb& beforeFirst, const Aabb& beforeSecond)
{
    const auto wrap = [](float value)
    {
        return value - std::round(value / SCREEN_W) * SCREEN_W;
    };
    const float oldX1 = (beforeFirst.l + beforeFirst.r) * 0.5f;
    const float oldX2 = (beforeSecond.l + beforeSecond.r) * 0.5f;
    const float oldY1 = (beforeFirst.t + beforeFirst.b) * 0.5f;
    const float oldY2 = (beforeSecond.t + beforeSecond.b) * 0.5f;
    const float nowX1 = first.m_x + first.m_width * 0.5f;
    const float nowX2 = second.m_x + second.m_width * 0.5f;
    const float old[] = { wrap(oldX2 - oldX1), oldY2 - oldY1 };
    const float now[] = {
        old[0] + wrap(nowX2 - oldX2) - wrap(nowX1 - oldX1),
        second.m_y + second.m_height * 0.5f - first.m_y - first.m_height * 0.5f
    };
    const float half[] = { (first.m_width + second.m_width) * 0.5f,
        (first.m_height + second.m_height) * 0.5f };
    const float epsilon = 0.001f;
    float enter = 0.0f, leave = 1.0f;
    int axisHit = -1;
    // 2人の相対移動区間を矩形へ通し、走ってすれ違うフレームも検出する。
    for (int axis = 0; axis < 2; ++axis)
    {
        const float delta = now[axis] - old[axis];
        if (std::fabs(delta) < epsilon)
        {
            if (std::fabs(old[axis]) > half[axis] + epsilon) return;
        }
        else
        {
            float a = (-half[axis] - old[axis]) / delta;
            float b = (half[axis] - old[axis]) / delta;
            if (a > b) { const float temp = a; a = b; b = temp; }
            if (a > enter) { enter = a; axisHit = axis; }
            leave = std::fmin(leave, b);
        }
        if (enter > leave + epsilon) return;
    }
    // 接触開始位置から離れるジャンプ・移動は引き戻さない。
    if (axisHit == -1 && (std::fabs(now[0]) > half[0] + epsilon
        || std::fabs(now[1]) > half[1] + epsilon)) return;
    bool vertical = axisHit == 1;
    if (axisHit == -1)
    {
        if (std::fabs(old[1]) >= half[1] - epsilon) vertical = true;
        else vertical = half[1] - std::fabs(now[1]) < half[0] - std::fabs(now[0]);
    }
    if (vertical && std::fabs(now[0]) < half[0] - epsilon)
    {
        Player& upper = old[1] >= 0.0f ? first : second;
        Player& lower = old[1] >= 0.0f ? second : first;
        const float lowerVelocityY = lower.m_vy;
        const float oldGap = std::fabs(old[1]) - half[1];
        if (lowerVelocityY < -epsilon && oldGap >= -epsilon
            && upper.m_contactLockTimer <= 0.0f)
        {
            // 下からの頭突き。上側を飛ばし、下側の上昇は止める。
            int direction = lower.m_facingLeft ? -1 : 1;
            const float offset = wrap(upper.m_x + upper.m_width * 0.5f
                - lower.m_x - lower.m_width * 0.5f);
            if (std::fabs(lower.m_vx) > epsilon) direction = lower.m_vx < 0.0f ? -1 : 1;
            else if (std::fabs(offset) > epsilon) direction = offset < 0.0f ? -1 : 1;
            lower.MoveByContact_(0.0f, upper.m_y + upper.m_height - lower.m_y);
            lower.m_vy = 0.0f;
            lower.m_isJumpHolding = false;
            lower.m_jumpHoldTimer = lower.m_jumpPoseTimer = 0.0f;
            upper.LaunchByPlayer_(direction);
        }
        else
        {
            // 上から降りた場合は頭の上に乗る。そこから通常のジャンプで離れられる。
            upper.MoveByContact_(0.0f, lower.m_y - upper.m_height - upper.m_y);
            if (std::fabs(upper.m_y + upper.m_height - lower.m_y) <= epsilon)
            {
                upper.m_vy = lower.m_vy;
                upper.m_isGrounded = lower.m_isGrounded;
                upper.m_isJumpHolding = false;
                upper.m_jumpHoldTimer = upper.m_jumpPoseTimer = 0.0f;
            }
        }
        // 頭上の足場や足元の床で縦補正できない場合は、横へ分ける。
        const float overlapY = std::fmin(first.m_y + first.m_height, second.m_y + second.m_height)
            - std::fmax(first.m_y, second.m_y);
        if (overlapY <= epsilon)
        {
            first.UpdateAnimation_(0.0f);
            second.UpdateAnimation_(0.0f);
            return;
        }
    }
    const float overlapY = std::fmin(first.m_y + first.m_height, second.m_y + second.m_height)
        - std::fmax(first.m_y, second.m_y);
    if (overlapY <= epsilon) return;
    const float side = old[0] >= 0.0f ? 1.0f : -1.0f;
    // 移動前の左右順を保つため、同フレームですれ違ってもすり抜けない。
    float distance = wrap(second.m_x + second.m_width * 0.5f - first.m_x - first.m_width * 0.5f);
    distance += std::round((now[0] - distance) / SCREEN_W) * SCREEN_W;
    float remaining = half[0] - side * distance;
    if (remaining <= epsilon) return;
    const float halfPush = remaining * 0.5f;
    remaining += side * first.MoveByContact_(-side * halfPush, 0.0f);
    remaining -= side * second.MoveByContact_(side * halfPush, 0.0f);
    // 一方が壁・POWで動けないなら、動ける側へ残りの補正を割り当てる。
    if (remaining > epsilon) remaining += side * first.MoveByContact_(-side * remaining, 0.0f);
    if (remaining > epsilon) second.MoveByContact_(side * remaining, 0.0f);
    first.UpdateAnimation_(0.0f);
    second.UpdateAnimation_(0.0f);
}

void Player::HandleMove_(const Input& input)
{
    if (m_contactLockTimer > 0.0f)
    {
        m_vx = m_contactVelocityX;
        m_isRunning = false;
        return;
    }
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
    if (m_contactLockTimer > 0.0f) return;
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
