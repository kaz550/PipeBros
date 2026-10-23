// 土管兄弟 ソース解説
// アプリ全体の初期化、毎フレームの入力・時間・シーン更新、終了処理を担当する。
// 入力の取得は1フレームに1回。個別のゲームルールはGameSceneへ委ねる。

#include "App.h"
#include "Config.h"
#include "../Input/Input.h"
#include "../Audio/Audio.h"
#include "../Scene/TitleScene.h"
#include "../Game/Score.h"
#include "DxLib.h"

#include <memory>
#include <cstdio>

// ウィンドウ・画面サイズ・裏画面描画を設定する。DxLibの初期化失敗ならゲームループへ進まない。
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

// 最後にDxLibを終了する。音声などの明示的な後片付けはこの前に済ませる。
void App::ShutdownDxLib_()
{
	DxLib_End();
}

// フレーム処理が短いときだけ待つ簡易リミッター。整数msで割るため厳密な60FPS同期ではない。
void App::LimitFps_()
{
	const int frameMs = 1000 / TARGET_FPS;
	static int prev = GetNowCount();

	const int now = GetNowCount();
	const int diff = now - prev;
	if (diff < frameMs) WaitTimer(frameMs - diff);

	prev = GetNowCount();
}

// 入力取得→dt更新→シーン更新→描画→画面反転の順を維持し、終了時に資源を後片付けする。
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
	// 未使用候補: hitは登録されるが、現行srcにPlaySe("hit")の呼出はない。
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
		// 取得はここだけ。以降のシーン・個体は同じフレームのスナップショットを参照する。
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

			// 注意: Forceは連続回数も0に戻す。この呼び方では毎サンプル履歴を消すため、
			// High復帰に必要な2回連続のカウントが蓄積しない。動作の修正は別途行う。
			m_quality.Force(m_scene.Context().quality);
			m_quality.UpdateOnFpsSample(fps);
			m_scene.Context().quality = m_quality.Level();
		}

		// Scene update/draw
		// Update内で要求されたシーン切替を適用してから、その新しいシーンを描画する。
		m_scene.Update(dt, input);
		if (m_scene.Context().quitRequested) break;

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
