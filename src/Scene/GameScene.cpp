#include "GameScene.h"
#include "../Input/Input.h"
#include "DxLib.h"

GameScene::GameScene(SceneManager* mgr)
{
    // シーン生成側とのインターフェースを維持する
    (void)mgr;
}

void GameScene::Enter()
{
    m_player.Reset(PLAYER_START_X, GROUND_Y);
}

void GameScene::Update(float dt, const Input& input)
{
    m_player.Update(dt, input);
}

void GameScene::Draw()
{
    m_player.Draw();

    const unsigned int white =
        GetColor(255, 255, 255);

    const unsigned int gray =
        GetColor(180, 180, 180);

    DrawString(
        24, 24,
        "PIPE BROS - PLAYER TEST",
        white
    );

    DrawString(
        24, 56,
        "LEFT / RIGHT : WALK",
        white
    );

    DrawString(
        24, 80,
        "SHIFT + LEFT / RIGHT : RUN",
        white
    );

    DrawString(
        24, 104,
        "SPACE : JUMP",
        white
    );

    DrawFormatString(
        24, 144,
        gray,
        "POSITION : %.1f, %.1f",
        m_player.GetX(),
        m_player.GetY()
    );

    DrawFormatString(
        24, 168,
        gray,
        "VELOCITY : %.1f, %.1f",
        m_player.GetVelocityX(),
        m_player.GetVelocityY()
    );

    DrawFormatString(
        24, 192,
        gray,
        "GROUNDED : %s",
        m_player.IsGrounded() ? "YES" : "NO"
    );

    DrawString(
        24, 650,
        "GROUND Y = 620  /  FLOOR IMAGE WILL BE ADDED WITH TILED",
        gray
    );
}