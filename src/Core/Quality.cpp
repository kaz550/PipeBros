#include "Quality.h"

void Quality::Reset(QualityLevel start)
{
	m_level = start;
	m_lowCount = 0;
	m_highCount = 0;
}

void Quality::Force(QualityLevel lv)
{
	m_level = lv;
	m_lowCount = 0;
	m_highCount = 0;
}

void Quality::UpdateOnFpsSample(float fps)
{
	if (fps < 30.0f)
	{
		m_lowCount++;
		m_highCount = 0;

		// 1秒(=1回)続いたらLOWへ
		if (m_level == QualityLevel::High && m_lowCount >= 1)
			m_level = QualityLevel::Low;

		return;
	}

	if (fps > 40.0f)
	{
		m_highCount++;
		m_lowCount = 0;

		// 2秒(=2回)続いたらHIGHへ
		if (m_level == QualityLevel::Low && m_highCount >= 2)
			m_level = QualityLevel::High;

		return;
	}

	// 中間帯：連続性をリセットしない（ゆらぎ抑制）
	m_lowCount = 0;
	m_highCount = 0;
}