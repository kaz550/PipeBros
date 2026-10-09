#include "App.h"
#include "Config.h"
#include "../Input/Input.h"
#include "../Audio/Audio.h"
#include "../Scene/TitleScene.h"
#include "../Game/Score.h"
#include "DxLib.h"

#include <memory>
#include <cstdio>

bool App::InitDxLib_()
{
	ChangeWindowMode(TRUE);
	SetGraphMode(SCREEN_W, SCREEN_H, 32);
	SetMainWindowText("Pipe Bros");

	if (DxLib_Init() == -1) return false;

	SetDrawScreen(DX_SCREEN_BACK);
	SetAlwaysRunFlag(TRUE);
	return true;
}

void App::ShutdownDxLib_()
{
	DxLib_End();
}

void App::LimitFps_()
{
	const int frameMs = 1000 / TARGET_FPS;
	static int prev = GetNowCount();

	const int now = GetNowCount();
	const int diff = now - prev;
	if (diff < frameMs) WaitTimer(frameMs - diff);

	prev = GetNowCount();
}

int App::Run()
{
	if (!InitDxLib_()) return -1;

	Input input;

	// Audio
	if (!Audio::Instance().Init())
	{
		ShutdownDxLib_();
		return -1;
	}

	Audio::Instance().LoadSe("assets/se_decide.wav", "decide");
	Audio::Instance().LoadSe("assets/se_back.wav", "back");
	Audio::Instance().LoadSe("assets/se_hit.wav", "hit");

	Audio::Instance().LoadBgm("assets/bgm.wav");
	Audio::Instance().PlayBgm(true);

	// HiScore load
	float hi = 0.0f;
	Score::LoadHiScore("hiscore.dat", hi);
	m_scene.Context().hiScore = hi;

	// Scene start
	m_scene.Set(std::make_unique<TitleScene>(&m_scene));

	// FPS / Quality init
	m_time.Reset();
	m_fps.Reset();
	m_quality.Reset(QualityLevel::High);

	m_scene.Context().showFps = true;
	m_scene.Context().fps = 60.0f;
	m_scene.Context().quality = QualityLevel::High;

	while (ProcessMessage() == 0)
	{
		input.Update();
		if (input.Triggered(Action::Quit)) break;

		m_time.Update();
		const float dt = m_time.DeltaTime();

		// fps sample update (about 1 sec)
		const bool fpsUpdated = m_fps.Update();
		if (fpsUpdated)
		{
			const float fps = m_fps.GetFps();
			m_scene.Context().fps = fps;

			// 手動でLOWにされた場合も「継続判定」は働くが、
			// ForceLowは“きっかけ”として残す運用（HIGH復帰は自動でOK）
			m_quality.Force(m_scene.Context().quality);
			m_quality.UpdateOnFpsSample(fps);
			m_scene.Context().quality = m_quality.Level();
		}

		// Scene update/draw
		m_scene.Update(dt, input);

		ClearDrawScreen();
		m_scene.Draw();

		// Common HUD
		if (m_scene.Context().showFps)
		{
			char fbuf[64];
			std::snprintf(fbuf, sizeof(fbuf), "FPS: %.1f", m_scene.Context().fps);
			DrawString(20, SCREEN_H - 30, fbuf, GetColor(180, 180, 180));
		}

		const char* qStr = (m_scene.Context().quality == QualityLevel::High) ? "QUALITY: HIGH" : "QUALITY: LOW";
		DrawString(SCREEN_W - 220, SCREEN_H - 30, qStr, GetColor(180, 180, 180));

		ScreenFlip();
		LimitFps_();
	}

	Audio::Instance().Shutdown();
	ShutdownDxLib_();
	return 0;
}