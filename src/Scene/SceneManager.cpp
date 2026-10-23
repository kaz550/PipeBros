// 土管兄弟 ソース解説
// 現在のシーンを所有し、要求された切替をUpdate終了後に適用する。
// Update実行中のシーンをその場で破棄せず、入力処理を最後まで安全に終える。

#include "SceneManager.h"
#include "IScene.h"

#include "TitleScene.h"
#include "GameScene.h"
#include "ResultScene.h"

#include <memory>

// 旧シーンをExitしてから所有権を置き換え、新シーンをEnterする。
void SceneManager::Set(std::unique_ptr<IScene> scene)
{
	if (m_scene) m_scene->Exit();
	m_scene = std::move(scene);
	if (m_scene) m_scene->Enter();
}

// 切替先を記録するだけ。呼出中のシーンはここでは破棄しない。
void SceneManager::RequestChange(SceneType next)
{
	m_hasRequest = true;
	m_next = next;
}

// Update後の安全なタイミングで要求されたシーンを生成する。
void SceneManager::ApplyChange_()
{
	if (!m_hasRequest) return;
	m_hasRequest = false;

	switch (m_next)
	{
	case SceneType::Title:
		Set(std::make_unique<TitleScene>(this));
		break;
	case SceneType::Game:
		Set(std::make_unique<GameScene>(this));
		break;
	case SceneType::Result:
		Set(std::make_unique<ResultScene>(this));
		break;
	}
}

// 現シーンの更新が終わってから遷移を適用する。
void SceneManager::Update(float dt, const Input& input)
{
	if (m_scene) m_scene->Update(dt, input);
	ApplyChange_();
}

// その時点で所有しているシーンだけを描画する。
void SceneManager::Draw()
{
	if (m_scene) m_scene->Draw();
}