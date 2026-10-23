// 土管兄弟 ソース解説
// 前回フレームとの差分を秒へ変換し、移動・アニメーション用のdtを供給する。
// 停止復帰直後に大きく移動しないよう、1回のdtを最大0.25秒へ制限する。

#pragma once

class Time
{
public:
	void Reset();
	void Update();

	float DeltaTime() const { return m_deltaTime; }
    // 未使用候補: 累計時間の取得元。現行のプレイ時間はGameSceneが別時計で管理する。
	float TotalTime() const { return m_totalTime; }

private:
	int   m_prevMs = 0;
	float m_deltaTime = 0.0f;
	float m_totalTime = 0.0f;
};