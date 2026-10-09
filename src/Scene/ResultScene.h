#pragma once
#include "IScene.h"

class SceneManager;

class ResultScene : public IScene
{
public:
	explicit ResultScene(SceneManager* mgr) : m_mgr(mgr) {}

	void Enter() override;
	void Update(float dt, const class Input& input) override;
	void Draw() override;

private:
	SceneManager* m_mgr = nullptr;
	bool m_isNew = false;
};