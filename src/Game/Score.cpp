// 土管兄弟 ソース解説
// ハイスコアをfloat値1個のバイナリファイルとして読み書きする。
// 得点の加算はGameScene、最大値の更新と保存要求はResultSceneが担当する。

#include "Score.h"
#include <fstream>

// float1個をバイナリから読み、失敗時は0を返す。戻り値は読込成否。
bool Score::LoadHiScore(const std::string& path, float& outHi)
{
	std::ifstream ifs(path, std::ios::binary);
	if (!ifs) { outHi = 0.0f; return false; }

	float v = 0.0f;
	ifs.read(reinterpret_cast<char*>(&v), sizeof(float));
	if (!ifs) { outHi = 0.0f; return false; }

	outHi = ClampScore(v);
	return true;
}

// 表示上限へ収めてからfloat1個を保存する。ゲーム中の個人得点はGameScene側で維持する。
bool Score::SaveHiScore(const std::string& path, float hi)
{
	hi = ClampScore(hi);

	std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
	if (!ofs) return false;

	ofs.write(reinterpret_cast<const char*>(&hi), sizeof(float));
	return (bool)ofs;
}

// 保存・表示用の数値を0〜999999へ収める。
float Score::ClampScore(float v)
{
	if (v < 0.0f) v = 0.0f;
	if (v > 999999.0f) v = 999999.0f;
	return v;
}