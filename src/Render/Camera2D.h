#pragma once
#include "../Game/Vec2.h"

class Camera2D
{
public:
    void Reset();

    void AddShake(float strengthPx, float timeSec);
    void Update(float dt);

    int OffsetX() const { return (int)m_offset.x; }
    int OffsetY() const { return (int)m_offset.y; }

private:
    Vec2  m_offset{ 0,0 };
    float m_shakeTime = 0.0f;
    float m_shakeDuration = 0.0f;
    float m_shakeStrength = 0.0f;
};