#pragma once

class Background
{
public:
    void Reset();
    void Update(float dt, float scrollSpeed);
    void Draw() const;

private:
    void DrawFarLayer_() const;
    void DrawMiddleLayer_() const;
    void DrawNearLayer_() const;

private:
    static constexpr int SCREEN_W = 1280;
    static constexpr int SCREEN_H = 720;
    static constexpr int GROUND_Y = 620;

    static constexpr float FAR_INTERVAL = 420.0f;
    static constexpr float MIDDLE_INTERVAL = 260.0f;
    static constexpr float NEAR_INTERVAL = 120.0f;

private:
    float m_farOffset = 0.0f;
    float m_middleOffset = 0.0f;
    float m_nearOffset = 0.0f;
};