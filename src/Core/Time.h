#pragma once

class Time
{
public:
	void Reset();
	void Update();

	float DeltaTime() const { return m_deltaTime; }
	float TotalTime() const { return m_totalTime; }

private:
	int   m_prevMs = 0;
	float m_deltaTime = 0.0f;
	float m_totalTime = 0.0f;
};