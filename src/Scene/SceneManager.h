// 土管兄弟 ソース解説
// 現在のシーンを所有し、要求された切替をUpdate終了後に適用する。
// Update実行中のシーンをその場で破棄せず、入力処理を最後まで安全に終える。

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