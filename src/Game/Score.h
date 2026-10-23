// 土管兄弟 ソース解説
// ハイスコアをfloat値1個のバイナリファイルとして読み書きする。
// 得点の加算はGameScene、最大値の更新と保存要求はResultSceneが担当する。

#pragma once
#include <string>

class Score
{
public:
	static bool LoadHiScore(const std::string& path, float& outHi);
	static bool SaveHiScore(const std::string& path, float hi);

	static float ClampScore(float v);
};