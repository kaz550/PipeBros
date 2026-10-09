#pragma once
#include "Aabb.h"

class Obstacle
{
public:
    Obstacle();
    Obstacle(float x, float groundY, float width, float height);

    void Update(float dt, float scrollSpeed);
    void Draw() const;

    Aabb GetAabb() const;

    bool IsAlive() const { return m_isAlive; }

    float GetX() const { return m_x; }
    float GetY() const { return m_y; }
    float GetWidth() const { return m_width; }
    float GetHeight() const { return m_height; }

private:
    float m_x = 0.0f;
    float m_y = 0.0f;
    float m_width = 48.0f;
    float m_height = 80.0f;
    float m_groundY = 620.0f;

    bool m_isAlive = true;
};