#pragma once
#include <string>
#include <unordered_map>

class Audio
{
public:
	static Audio& Instance();

	bool Init();
	void Shutdown();

	// SE
	bool LoadSe(const std::string& path, const std::string& key);
	void PlaySe(const std::string& key);

	// BGM
	bool LoadBgm(const std::string& path);
	void PlayBgm(bool loop = true);
	void PlayBgm(const std::string& path, bool loop = true);
	void StopBgm();

private:
	Audio() = default;

	std::unordered_map<std::string, int> m_se; // key -> handle
	int m_bgm = -1;
};