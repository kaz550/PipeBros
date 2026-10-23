// 土管兄弟 ソース解説
// FPSサンプルに応じてHigh/Lowの品質状態を管理する。
// 現在のゲーム描画はこの状態で品質を切り替えず、AppのQUALITY表示へ反映する。

#include "Quality.h"

// 開始品質と連続回数を初期化する。
void Quality::Reset(QualityLevel start)
{
	m_level = start;
	m_lowCount = 0;
	m_highCount = 0;
}

// 指定品質へ強制設定し、連続回数も0に戻す。毎サンプル呼ぶと前回の回数を引き継げない。
void Quality::Force(QualityLevel lv)
{
	m_level = lv;
	m_lowCount = 0;
	m_highCount = 0;
}

// 30未満ならLow、40超が2回ならHighへ変更する想定。AppからのForce呼出で回数が消える点は一覧を参照。
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

	// 中間帯では両カウンタを0に戻す。閾値をまたいだ回数を連続として数えない。
	m_lowCount = 0;
	m_highCount = 0;
}