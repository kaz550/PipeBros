#include "Score.h"
#include <fstream>

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

bool Score::SaveHiScore(const std::string& path, float hi)
{
	hi = ClampScore(hi);

	std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
	if (!ofs) return false;

	ofs.write(reinterpret_cast<const char*>(&hi), sizeof(float));
	return (bool)ofs;
}

float Score::ClampScore(float v)
{
	if (v < 0.0f) v = 0.0f;
	if (v > 999999.0f) v = 999999.0f;
	return v;
}