#pragma once
#include <string>

class Score
{
public:
	static bool LoadHiScore(const std::string& path, float& outHi);
	static bool SaveHiScore(const std::string& path, float hi);

	static float ClampScore(float v);
};