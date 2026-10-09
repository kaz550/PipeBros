#include "Camera2D.h"
#include "DxLib.h"

void Camera2D::Reset()
{
	m_offset = Vec2(0, 0);
	m_shakeTime = 0.0f;
	m_shakeDuration = 0.0f;
	m_shakeStrength = 0.0f;
}

void Camera2D::AddShake(float strengthPx, float timeSec)
{
	if (timeSec > (m_shakeDuration - m_shakeTime))
	{
		m_shakeDuration = timeSec;
		m_shakeTime = 0.0f;
	}
	if (strengthPx > m_shakeStrength) m_shakeStrength = strengthPx;
}

void Camera2D::Update(float dt)
{
	if (m_shakeTime < m_shakeDuration)
	{
		m_shakeTime += dt;

		const float t = (m_shakeDuration <= 0.0f) ? 1.0f : (m_shakeTime / m_shakeDuration);
		const float strength = m_shakeStrength * (1.0f - t);

		const int sx = GetRand((int)(strength * 2)) - (int)strength;
		const int sy = GetRand((int)(strength * 2)) - (int)strength;

		m_offset = Vec2((float)sx, (float)sy);

		if (m_shakeTime >= m_shakeDuration)
		{
			m_offset = Vec2(0, 0);
			m_shakeStrength = 0.0f;
		}
	}
	else
	{
		m_offset = Vec2(0, 0);
	}
}