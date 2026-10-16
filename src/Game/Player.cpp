#include "Player.h"
#include "../Input/Input.h"
#include "DxLib.h"

Player::Player()
{
    // コンストラクタでは画像を読み込まない。
    // DxLib初期化後に呼ばれるReset()で読み込む。
}

Player::~Player()
{
    // Appの終了順によっては、DxLib_End()の後に
    // Playerが破棄される場合がある。
    // その場合、画像はDxLib_End()ですでに解放されている。
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
    // 読み込み済みなら再利用する
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
            "Failed to load: assets/images/pipe_bros.png\n"
        );
        return false;
    }

    int imageWidth = 0;
    int imageHeight = 0;

    if (GetGraphSize(
        m_spriteSheet,
        &imageWidth,
        &imageHeight
    ) == -1)
    {
        printfDx("Failed to get sprite sheet size.\n");

        DeleteGraph(m_spriteSheet);
        m_spriteSheet = -1;
        return false;
    }

    // 切り出し座標は、添付された1536×1024画像用
    if (imageWidth != 1536 || imageHeight != 1024)
    {
        printfDx(
            "pipe_bros.png must be 1536 x 1024 pixels.\n"
        );

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

    // 今回はアイドル1コマ目を表示
    m_frameIndex = 0;
}

void Player::Update(float dt, const Input& input)
{
    // 既存のインターフェースに合わせて引数を残す。
    // 今回のジャンプ入力はCheckHitKey()で取得する。
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

    // x, y, 幅, 高さ, 基準点X, 基準点Y
    //
    // 添付されたスプライトシートに合わせて
    // 各コマの切り出し範囲を個別に指定する。
    //
    // 基準点は、キャラクターの下端中央を初期値とする。
    // ポーズによる横ずれはpivotXで調整できる。
    static const SpriteFrame frames[] =
    {
        // アイドル：0～1
        {  82,  57, 150, 202,  75.0f, 200.0f },
        { 322,  55, 145, 204,  72.5f, 202.0f },

        // 歩き：2～7
        {  71, 300, 170, 206,  85.0f, 204.0f },
        { 335, 300, 139, 206,  69.5f, 204.0f },
        { 573, 297, 170, 210,  85.0f, 208.0f },
        { 814, 296, 165, 211,  82.5f, 209.0f },
        {1067, 294, 147, 213,  73.5f, 211.0f },
        {1308, 297, 161, 209,  80.5f, 207.0f },

        // 走り：8～13
        {  54, 545, 204, 202, 102.0f, 200.0f },
        { 294, 540, 199, 203,  99.5f, 201.0f },
        { 550, 544, 204, 199, 102.0f, 197.0f },
        { 797, 546, 204, 204, 102.0f, 202.0f },
        {1066, 547, 184, 200,  92.0f, 198.0f },
        {1293, 539, 214, 204, 107.0f, 202.0f },

        // ジャンプ：14～19
        {  71, 823, 169, 173,  84.5f, 171.0f },
        { 322, 771, 186, 225,  93.0f, 223.0f },
        { 568, 757, 175, 232,  87.5f, 230.0f },
        { 807, 768, 186, 192,  93.0f, 190.0f },
        {1050, 778, 181, 212,  90.5f, 210.0f },
        {1314, 823, 171, 173,  85.5f, 171.0f },
    };

    constexpr int frameCount =
        static_cast<int>(
            sizeof(frames) / sizeof(frames[0])
            );

    if (m_frameIndex < 0 ||
        m_frameIndex >= frameCount)
    {
        return;
    }

    const SpriteFrame& frame = frames[m_frameIndex];

    // 当たり判定の足元中央を、描画の基準にする
    const float anchorX = m_x + m_width * 0.5f;
    const float anchorY = m_y + m_height;

    // 各コマの基準点がanchorに重なる位置を求める
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
}

Aabb Player::GetAabb() const
{
    Aabb aabb;

    // 見た目より少し小さくする
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
    const bool jumpKey =
        CheckHitKey(KEY_INPUT_SPACE) != 0;

    const bool jumpPressed =
        jumpKey && !m_prevJumpKey;

    const bool jumpReleased =
        !jumpKey && m_prevJumpKey;

    // 接地中に押した瞬間だけジャンプする
    if (m_isGrounded && jumpPressed)
    {
        m_vy = m_jumpVelocity;
        m_isGrounded = false;
        m_isJumpHolding = true;
        m_jumpHoldTimer = 0.0f;
    }

    // 押し続けている間の処理
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

    // 上昇中に離したら、上昇速度を弱める
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