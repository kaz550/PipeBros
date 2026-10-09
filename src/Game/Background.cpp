#include "Background.h"
#include "DxLib.h"

void Background::Reset()
{
    m_farOffset = 0.0f;
    m_middleOffset = 0.0f;
    m_nearOffset = 0.0f;
}

void Background::Update(float dt, float scrollSpeed)
{
    m_farOffset += scrollSpeed * 0.15f * dt;
    m_middleOffset += scrollSpeed * 0.35f * dt;
    m_nearOffset += scrollSpeed * 0.75f * dt;

    if (m_farOffset >= FAR_INTERVAL)
    {
        m_farOffset -= FAR_INTERVAL;
    }

    if (m_middleOffset >= MIDDLE_INTERVAL)
    {
        m_middleOffset -= MIDDLE_INTERVAL;
    }

    if (m_nearOffset >= NEAR_INTERVAL)
    {
        m_nearOffset -= NEAR_INTERVAL;
    }
}

void Background::Draw() const
{
    DrawBox(0, 0, SCREEN_W, SCREEN_H, GetColor(18, 22, 34), TRUE);

    DrawFarLayer_();
    DrawMiddleLayer_();
    DrawNearLayer_();
}

void Background::DrawFarLayer_() const
{
    const int mountainColor = GetColor(34, 44, 68);
    const int lineColor = GetColor(50, 62, 88);

    for (float x = -m_farOffset; x < SCREEN_W + FAR_INTERVAL; x += FAR_INTERVAL)
    {
        const int ix = static_cast<int>(x);

        DrawTriangle(
            ix + 40, 430,
            ix + 210, 240,
            ix + 380, 430,
            mountainColor,
            TRUE
        );

        DrawLine(ix + 210, 240, ix + 380, 430, lineColor);
    }
}

void Background::DrawMiddleLayer_() const
{
    const int cloudColor = GetColor(46, 58, 82);
    const int pillarColor = GetColor(42, 52, 74);

    for (float x = -m_middleOffset; x < SCREEN_W + MIDDLE_INTERVAL; x += MIDDLE_INTERVAL)
    {
        const int ix = static_cast<int>(x);

        DrawCircle(ix + 60, 180, 22, cloudColor, TRUE);
        DrawCircle(ix + 90, 170, 30, cloudColor, TRUE);
        DrawCircle(ix + 125, 182, 20, cloudColor, TRUE);
        DrawBox(ix + 55, 180, ix + 135, 205, cloudColor, TRUE);

        DrawBox(ix + 190, 360, ix + 220, GROUND_Y, pillarColor, TRUE);
        DrawBox(ix + 180, 340, ix + 230, 365, pillarColor, TRUE);
    }
}

void Background::DrawNearLayer_() const
{
    const int grassColor = GetColor(48, 84, 58);
    const int lineColor = GetColor(64, 110, 74);

    for (float x = -m_nearOffset; x < SCREEN_W + NEAR_INTERVAL; x += NEAR_INTERVAL)
    {
        const int ix = static_cast<int>(x);

        DrawLine(ix + 10, GROUND_Y, ix + 24, GROUND_Y - 28, grassColor);
        DrawLine(ix + 24, GROUND_Y, ix + 34, GROUND_Y - 18, lineColor);
        DrawLine(ix + 46, GROUND_Y, ix + 60, GROUND_Y - 24, grassColor);
    }
}