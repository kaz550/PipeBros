// 土管兄弟 ソース解説
// 前回フレームとの差分を秒へ変換し、移動・アニメーション用のdtを供給する。
// 停止復帰直後に大きく移動しないよう、1回のdtを最大0.25秒へ制限する。

#include "Time.h"
#include "DxLib.h"

// フレーム差分の基準時刻を置き、経過秒を0へ戻す。
void Time::Reset()
{
	m_prevMs = GetNowCount();
	m_deltaTime = 0.0f;
	m_totalTime = 0.0f;
}

// ミリ秒を秒へ換算する。移動量は速度px/秒にこのdtを掛けて求める。
void Time::Update()
{
	const int now = GetNowCount();
	const int diff = now - m_prevMs;
	m_prevMs = now;

	m_deltaTime = diff / 1000.0f; // ms -> sec
	if (m_deltaTime < 0.0f) m_deltaTime = 0.0f;
	if (m_deltaTime > 0.25f) m_deltaTime = 0.25f; // Alt+Tab等の暴発防止

	m_totalTime += m_deltaTime;
}