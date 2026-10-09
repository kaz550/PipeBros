#pragma once
#include <memory>

#include "IScene.h"
#include "SceneType.h"
#include "SceneContext.h"

class IScene;
class Input;

class SceneManager
{
public:
	void Set(std::unique_ptr<IScene> scene);

	void RequestChange(SceneType next);

	void Update(float dt, const Input& input);
	void Draw();

	SceneContext& Context() { return m_ctx; }
	const SceneContext& Context() const { return m_ctx; }

private:
	void ApplyChange_();

private:
	std::unique_ptr<IScene> m_scene;

	bool m_hasRequest = false;
	SceneType m_next = SceneType::Title;

	SceneContext m_ctx{};
};