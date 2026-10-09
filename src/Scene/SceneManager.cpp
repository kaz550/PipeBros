#include "SceneManager.h"
#include "IScene.h"

#include "TitleScene.h"
#include "GameScene.h"
#include "ResultScene.h"

#include <memory>

void SceneManager::Set(std::unique_ptr<IScene> scene)
{
	if (m_scene) m_scene->Exit();
	m_scene = std::move(scene);
	if (m_scene) m_scene->Enter();
}

void SceneManager::RequestChange(SceneType next)
{
	m_hasRequest = true;
	m_next = next;
}

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

void SceneManager::Update(float dt, const Input& input)
{
	if (m_scene) m_scene->Update(dt, input);
	ApplyChange_();
}

void SceneManager::Draw()
{
	if (m_scene) m_scene->Draw();
}