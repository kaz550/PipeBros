#include "ResultScene.h"
#include "SceneManager.h"
#include "SceneType.h"

#include "../Input/Input.h"
#include "../Audio/Audio.h"
#include "../Core/Config.h"
#include "../Game/Score.h"
#include "../Core/Quality.h"

#include "DxLib.h"
#include <cstdio>

static const char* ResultTitle_(ResultKind k)
{
	switch (k)
	{
	case ResultKind::GameOver:   return "GAME OVER";
	case ResultKind::StageClear: return "STAGE CLEAR";
	case ResultKind::AllClear:   return "ALL CLEAR";
	default:                     return "RESULT";
	}
}

void ResultScene::Enter()
{
	m_isNew = false;

	const float score = m_mgr->Context().lastScore;
	if (score > m_mgr->Context().hiScore)
	{
		m_mgr->Context().hiScore = score;
		Score::SaveHiScore("hiscore.dat", m_mgr->Context().hiScore);
		m_isNew = true;
	}
}

void ResultScene::Update(float, const Input& input)
{
	if (input.Triggered(Action::ToggleFps))
		m_mgr->Context().showFps = !m_mgr->Context().showFps;

	if (input.Triggered(Action::ForceLow))
		m_mgr->Context().quality = QualityLevel::Low;

	if (input.Triggered(Action::Decide))
	{
		Audio::Instance().PlaySe("decide");
		m_mgr->RequestChange(SceneType::Game);
		return;
	}

	if (input.Triggered(Action::Back))
	{
		Audio::Instance().PlaySe("back");
		m_mgr->RequestChange(SceneType::Title);
		return;
	}
}

void ResultScene::Draw()
{
	DrawBox(0, 0, SCREEN_W, SCREEN_H, GetColor(12, 8, 8), TRUE);

	const int white = GetColor(240, 240, 240);
	const int gray = GetColor(180, 180, 180);
	const int gold = GetColor(255, 240, 160);

	DrawString(40, 40, ResultTitle_(m_mgr->Context().resultKind), white);

	DrawString(40, 90, "Decide: Next (-> Game)", gray);
	DrawString(40, 120, "Back  : Title (-> Title)", gray);

	char buf[128];
	std::snprintf(buf, sizeof(buf), "SCORE: %.0f", m_mgr->Context().lastScore);
	DrawString(40, 200, buf, white);

	std::snprintf(buf, sizeof(buf), "HI-SCORE: %.0f", m_mgr->Context().hiScore);
	DrawString(40, 240, buf, white);

	std::snprintf(buf, sizeof(buf), "NEXT STAGE: %d / %d", m_mgr->Context().stageIndex + 1, m_mgr->Context().stageCount);
	DrawString(40, 280, buf, gray);



	if (m_isNew)
		DrawString(40, 330, "NEW RECORD!", gold);
}