#pragma once
#include "IScene.h"
#include "SceneContext.h"
#include "../Game/Player.h"
#include "../Game/ObstacleManager.h"
#include "../Game/Background.h"

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
    void UpdateGame_(float dt, const Input& input);
    void UpdateDifficulty_(float dt);
    void UpdateScroll_(float dt);
    void DrawGround_() const;
    void DrawHud_() const;
    void DrawGameOver_() const;
    void RequestGameOver_();

private:
    static constexpr int SCREEN_W = 1280;
    static constexpr int SCREEN_H = 720;
    static constexpr int GROUND_Y = 620;

    static constexpr float PLAYER_START_X = 220.0f;
    static constexpr float BASE_SCROLL_SPEED = 220.0f;
    static constexpr float SPEED_UP_RATE = 9.0f;
    static constexpr float MAX_SCROLL_SPEED = 560.0f;
    static constexpr float GROUND_PATTERN_INTERVAL = 64.0f;
    static constexpr float MAX_DIFFICULTY_TIME = 90.0f;

private:
    SceneManager* m_mgr = nullptr;

    Background m_background;
    Player m_player;
    ObstacleManager m_obstacleManager;

    float m_distance = 0.0f;
    float m_scrollSpeed = BASE_SCROLL_SPEED;
    float m_scrollOffset = 0.0f;
    float m_elapsedTime = 0.0f;
    float m_difficulty = 0.0f;

    bool m_isGameOver = false;
};