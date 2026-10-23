// 土管兄弟 ソース解説
// 生成するシーンをTitle/Game/Resultで指定する列挙型。
// 生成処理はSceneManager::ApplyChange_に集約する。

#pragma once

enum class SceneType
{
	Title,
	Game,
	Result
};