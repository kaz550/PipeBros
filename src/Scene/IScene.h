// 土管兄弟 ソース解説
// Title/Game/Resultで共通のシーン操作を定義する基底クラス。
// SceneManagerがEnter/ExitとUpdate/Drawを呼ぶ。virtualな関数は直接の文字列参照だけで未使用と判断しない。

#pragma once

class Input;

class IScene
{
public:
	virtual ~IScene() = default;

	// Enter/Exitはシーンの開始と後片付けのフック。未overrideのExitでも共通の呼出経路に必要。
	virtual void Enter() {}
	virtual void Exit() {}

	virtual void Update(float dt, const Input& input) = 0;
	virtual void Draw() = 0;
};