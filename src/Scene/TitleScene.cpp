#include "TitleScene.h"
#include "SceneManager.h"
#include "SceneType.h"

#include "../Input/Input.h"
#include "../Audio/Audio.h"
#include "../Core/Config.h"

#include "DxLib.h"
#include <cstdio>

void TitleScene::Enter()
{
	m_blink = 0.0f;
	m_showPress = true;
}

void TitleScene::Update(float dt, const Input& input)
{
	m_blink += dt;
	if (m_blink >= 0.5f)
	{
		m_blink = 0.0f;
		m_showPress = !m_showPress;
	}

	if (input.Triggered(Action::ToggleFps))
		m_mgr->Context().showFps = !m_mgr->Context().showFps;

	if (input.Triggered(Action::ForceLow))
		if (m_mgr->Context().quality == QualityLevel::High) {
			m_mgr->Context().quality = QualityLevel::Low;
		}
		else {
			m_mgr->Context().quality = QualityLevel::High;
		}

	if (input.Triggered(Action::Decide))
	{
		Audio::Instance().PlaySe("decide");
		m_mgr->RequestChange(SceneType::Game);
		return;
	}
}

void TitleScene::Draw()
{
	DrawBox(0, 0, SCREEN_W, SCREEN_H, GetColor(10, 12, 18), TRUE);

	for (int y = 0; y < SCREEN_H; y += 40)
		DrawLine(0, y, SCREEN_W, y, GetColor(16, 20, 30));

	const int white = GetColor(255, 255, 255);
	const int gray = GetColor(180, 180, 180);

	DrawString(40, 40, "Pipe Bros", gray);
	DrawString(40, 100, "Pipe Bros", white);

	DrawString(40, 170, "Jump: Space / PadA", gray);
	DrawString(40, 200, "Hold Jump: Higher jump", gray);
	DrawString(40, 230, "Rule: Avoid triangle obstacles", gray);
	DrawString(40, 260, "Score: Distance", gray);
	DrawString(40, 290, "Decide: Launch", gray);
	DrawString(40, 320, "Back: Esc/PadB (Debug)  F1: Toggle FPS  F2: Toggle LOW", gray);

	char buf[128];
	std::snprintf(buf, sizeof(buf), "HI-SCORE: %.2f", m_mgr->Context().hiScore);
	DrawString(40, 280, buf, white);

	if (m_showPress)
		DrawString(40, 360, "PRESS DECIDE TO START", white);
}