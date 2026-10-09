#pragma once
#include "../Scene/SceneManager.h"
#include "Time.h"
#include "Fps.h"
#include "Quality.h"

class App
{
public:
	int Run();

private:
	bool InitDxLib_();
	void ShutdownDxLib_();
	void LimitFps_();

private:
	SceneManager m_scene;
	Time         m_time;

	Fps          m_fps;
	Quality      m_quality;
};