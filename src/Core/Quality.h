#pragma once

enum class QualityLevel
{
	High,
	Low
};

class Quality
{
public:
	void Reset(QualityLevel start = QualityLevel::High);

	// 1秒平均fpsが更新されたタイミングで呼ぶ想定
	void UpdateOnFpsSample(float fps);

	void Force(QualityLevel lv);

	QualityLevel Level() const { return m_level; }

private:
	QualityLevel m_level = QualityLevel::High;

	// 「fpsサンプル回数」で継続判定（Fpsが1秒更新のため）
	int m_lowCount = 0;   // fps < 30 の連続回数
	int m_highCount = 0;  // fps > 40 の連続回数
};