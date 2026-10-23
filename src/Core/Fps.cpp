// 土管兄弟 ソース解説
// 実際に描画したフレーム数から、約1秒ごとの平均FPSを求める。
// FPSの計測担当。フレーム待ちはApp::LimitFps_、移動用の秒数はTimeが担当する。

#include "Fps.h"
#include "DxLib.h"

// 計測開始時刻とフレーム数を初期化する。
void Fps::Reset()
{
	m_prevMs = GetNowCount();
	m_accMs = 0;
	m_frames = 0;
	m_fps = 60.0f;
}

// 約1000ms分のフレーム数を集計したときだけtrueを返し、品質判定用のFPSを更新する。
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