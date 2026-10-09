#include "Obstacle.h"
#include "DxLib.h"

Obstacle::Obstacle()
{
}

Obstacle::Obstacle(float x, float groundY, float width, float height)
    : m_x(x)
    , m_width(width)
    , m_height(height)
    , m_groundY(groundY)
    , m_isAlive(true)
{
    m_y = m_groundY - m_height;
}

void Obstacle::Update(float dt, float scrollSpeed)
{
    m_x -= scrollSpeed * dt;

    if (m_x + m_width < 0.0f)
    {
        m_isAlive = false;
    }
}

void Obstacle::Draw() const
{
    const int left = static_cast<int>(m_x);
    const int right = static_cast<int>(m_x + m_width);
    const int top = static_cast<int>(m_y);
    const int bottom = static_cast<int>(m_groundY);
    const int centerX = static_cast<int>(m_x + m_width * 0.5f);

    DrawTriangle(
        centerX, top,
        left, bottom,
        right, bottom,
        GetColor(255, 190, 80),
        TRUE
    );

    DrawTriangle(
        centerX, top,
        left, bottom,
        right, bottom,
        GetColor(255, 240, 180),
        FALSE
    );
}

Aabb Obstacle::GetAabb() const
{
    Aabb aabb;

    // éOäpå`ÇÃå©ÇΩñ⁄Ç…ëŒÇµÇƒÅAè≠Çµì‡ë§Ç…îªíËÇçÏÇÈ
    const float marginX = 8.0f;
    const float marginTop = 10.0f;

    aabb.l = m_x + marginX;
    aabb.t = m_y + marginTop;
    aabb.r = m_x + m_width - marginX;
    aabb.b = m_groundY;

    return aabb;
}