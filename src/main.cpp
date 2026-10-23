// 土管兄弟 ソース解説
// Windowsのエントリーポイント。Appを作り、初期化から終了までRunへ任せる。
// ここには個別のゲームルールを置かず、App/SceneManagerから各処理へ進む。

#include "Core/App.h"
#include "DxLib.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	App app;
	return app.Run();
}