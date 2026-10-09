#pragma once
#include "IScene.h"

class SceneManager;

class TitleScene : public IScene
{
public:
	explicit TitleScene(SceneManager* mgr) : m_mgr(mgr) {}

	void Enter() override;
	void Update(float dt, const class Input& input) override;
	void Draw() override;

private:
	SceneManager* m_mgr = nullptr;
	float m_blink = 0.0f;
	bool  m_showPress = true;
};