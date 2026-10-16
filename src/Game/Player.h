#pragma once

#include "Aabb.h"

class Input;

class Player
{
public:
    Player();
    ~Player();

    // 画像ハンドルの二重解放を防ぐ
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
    float GetVelocityY() const { return m_vy; }

    bool IsGrounded() const { return m_isGrounded; }
    bool IsJumpHolding() const { return m_isJumpHolding; }

private:
    // スプライトシート内の各コマの情報
    struct SpriteFrame
    {
        int x;
        int y;
        int width;
        int height;

        // 切り出した画像の左上から見た描画基準点
        float pivotX;
        float pivotY;
    };

    bool LoadSprite_();

    void HandleJump_(float dt);
    void ApplyGravity_(float dt);
    void UpdatePosition_(float dt);
    void ClampToGround_();

private:
    // 当たり判定の左上座標
    float m_x = 0.0f;
    float m_y = 0.0f;

    float m_vy = 0.0f;

    // 当たり判定のサイズ。
    // スプライトの切り出しサイズとは独立して管理する
    float m_width = 40.0f;
    float m_height = 80.0f;

    float m_gravity = 2200.0f;
    float m_jumpVelocity = -820.0f;

    // ジャンプキーを離した際の上昇速度の倍率
    float m_lowJumpRate = 0.45f;

    // ジャンプキーを押している間の重力倍率
    float m_jumpHoldGravityRate = 0.45f;

    float m_maxJumpHoldTime = 0.18f;
    float m_jumpHoldTimer = 0.0f;

    float m_groundY = 620.0f;

    bool m_isGrounded = false;
    bool m_isJumpHolding = false;
    bool m_prevJumpKey = false;

    // スプライトシート
    int m_spriteSheet = -1;

    // 表示するコマ番号
    // アイドル：0～1
    // 歩き：2～7
    // 走り：8～13
    // ジャンプ：14～19
    int m_frameIndex = 0;

    // 全コマ共通の描画倍率
    float m_spriteScale = 0.4f;
};