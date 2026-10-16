#pragma once

#include "IScene.h"
#include "../Game/Player.h"

class SceneManager;
class Input;

class GameScene : public IScene
{
public:
    explicit GameScene(SceneManager* mgr);

    void Enter() override;
    void Update(float dt, const Input& input) override;
    void Draw() override;

private:
    static constexpr float GROUND_Y = 620.0f;
    static constexpr float PLAYER_START_X = 220.0f;

    Player m_player;
};