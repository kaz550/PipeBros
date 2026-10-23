// 土管兄弟 ソース解説
// アプリ全体の初期化、毎フレームの入力・時間・シーン更新、終了処理を担当する。
// 入力の取得は1フレームに1回。個別のゲームルールはGameSceneへ委ねる。

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