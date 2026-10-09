#include "Time.h"
#include "DxLib.h"

void Time::Reset()
{
	m_prevMs = GetNowCount();
	m_deltaTime = 0.0f;
	m_totalTime = 0.0f;
}

void Time::Update()
{
	const int now = GetNowCount();
	const int diff = now - m_prevMs;
	m_prevMs = now;

	m_deltaTime = diff / 1000.0f; // ms -> sec
	if (m_deltaTime < 0.0f) m_deltaTime = 0.0f;
	if (m_deltaTime > 0.25f) m_deltaTime = 0.25f; // Alt+Tab“™‚Ì–\”­–hŽ~

	m_totalTime += m_deltaTime;
}