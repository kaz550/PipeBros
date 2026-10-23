// 土管兄弟 ソース解説
// SEとBGMのハンドルを一括管理する。Appが初期化・終了し、各シーンが再生を要求する。
// 画像と同様、音声も読み込んだハンドルを保持し、Shutdownで解放する。

#include "Audio.h"
#include "../Core/Config.h"
#include "DxLib.h"

// 全シーンで共有する音声管理オブジェクトを返す。所有者を増やさずハンドルを一元化する。
Audio& Audio::Instance()
{
	static Audio inst;
	return inst;
}

// 初期化の入口。現状は常にtrueを返すフックで、音声ファイルの読込はLoadSe/LoadBgmが行う。
bool Audio::Init()
{
	return true;
}

// BGMを停止し、BGM・SEの全ハンドルを解放する。DxLib終了より先に呼ぶ。
void Audio::Shutdown()
{
	if (m_bgm >= 0)
	{
		StopSoundMem(m_bgm);
		DeleteSoundMem(m_bgm);
		m_bgm = -1;
	}

	for (auto& kv : m_se)
	{
		if (kv.second >= 0) DeleteSoundMem(kv.second);
	}
	m_se.clear();
}

// パスからSEを読み、文字列キーで登録する。失敗時はfalseを返し、再生要求は未登録キーなら無視される。
bool Audio::LoadSe(const std::string& path, const std::string& key)
{
	const int h = LoadSoundMem(path.c_str());
	if (h < 0) return false;

	ChangeVolumeSoundMem(SE_VOL, h);
	m_se[key] = h;
	return true;
}

// 登録済みのSEを再生要求する。呼出側はファイルパスではなくdecide/back等のキーを指定する。
void Audio::PlaySe(const std::string& key)
{
	auto it = m_se.find(key);
	if (it == m_se.end()) return;

	// 登録された音声ハンドルへ、バックグラウンドでの再生を要求する。
	PlaySoundMem(it->second, DX_PLAYTYPE_BACK);
}

// 前のBGMを解放してから次の曲を読む。現在は同時に1曲のハンドルだけを保持する。
bool Audio::LoadBgm(const std::string& path)
{
	if (m_bgm >= 0)
	{
		StopSoundMem(m_bgm);
		DeleteSoundMem(m_bgm);
		m_bgm = -1;
	}

	m_bgm = LoadSoundMem(path.c_str());
	if (m_bgm < 0) return false;

	ChangeVolumeSoundMem(BGM_VOL, m_bgm);
	return true;
}

// 既に読み込んだ曲の再生、またはパス指定で読込と再生をまとめて行う。loopでループの有無を選ぶ。
void Audio::PlayBgm(bool loop)
{
	if (m_bgm < 0) return;

	StopSoundMem(m_bgm);
	PlaySoundMem(m_bgm, loop ? DX_PLAYTYPE_LOOP : DX_PLAYTYPE_BACK);
}

// 既に読み込んだ曲の再生、またはパス指定で読込と再生をまとめて行う。loopでループの有無を選ぶ。
void Audio::PlayBgm(const std::string& path, bool loop)
{
	if (LoadBgm(path)) {
		PlayBgm(loop);
	}
}

// 再生を止めるがハンドルは残す。未使用候補: 添付src内に外部からの呼出はない。
void Audio::StopBgm()
{
	if (m_bgm < 0) return;
	StopSoundMem(m_bgm);
}