#include "Fps.h"
#include "DxLib.h"

void Fps::Reset()
{
	m_prevMs = GetNowCount();
	m_accMs = 0;
	m_frames = 0;
	m_fps = 60.0f;
}

bool Fps::Update()
{
	const int now = GetNowCount();
	const int diff = now - m_prevMs;
	m_prevMs = now;

	m_accMs += diff;
	m_frames += 1;

	if (m_accMs >= 1000)
	{
		const float sec = m_accMs / 1000.0f;
		m_fps = (sec > 0.0f) ? (m_frames / sec) : 60.0f;

		m_accMs = 0;
		m_frames = 0;
		return true;
	}

	return false;
}