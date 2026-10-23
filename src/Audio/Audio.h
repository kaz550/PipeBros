// 土管兄弟 ソース解説
// SEとBGMのハンドルを一括管理する。Appが初期化・終了し、各シーンが再生を要求する。
// 画像と同様、音声も読み込んだハンドルを保持し、Shutdownで解放する。

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
    // 未使用候補: パス指定の一括再生は現行から呼ばれない。LoadBgm+PlayBgm(bool)は使用中。
	void PlayBgm(const std::string& path, bool loop = true);
	void StopBgm();

private:
	Audio() = default;

	std::unordered_map<std::string, int> m_se; // key -> handle
	int m_bgm = -1;
};