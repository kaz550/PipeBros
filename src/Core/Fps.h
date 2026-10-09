#pragma once

class Fps
{
public:
	void Reset();

	// 戻り値：fps値を更新した(=約1秒経過した)ならtrue
	bool Update();

	float GetFps() const { return m_fps; }

private:
	int   m_prevMs = 0;
	int   m_accMs = 0;
	int   m_frames = 0;
	float m_fps = 60.0f;
};