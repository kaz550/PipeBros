#include "Audio.h"
#include "../Core/Config.h"
#include "DxLib.h"

Audio& Audio::Instance()
{
	static Audio inst;
	return inst;
}

bool Audio::Init()
{
	return true;
}

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

bool Audio::LoadSe(const std::string& path, const std::string& key)
{
	const int h = LoadSoundMem(path.c_str());
	if (h < 0) return false;

	ChangeVolumeSoundMem(SE_VOL, h);
	m_se[key] = h;
	return true;
}

void Audio::PlaySe(const std::string& key)
{
	auto it = m_se.find(key);
	if (it == m_se.end()) return;

	// ‘½dÄ¶
	PlaySoundMem(it->second, DX_PLAYTYPE_BACK);
}

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

void Audio::PlayBgm(bool loop)
{
	if (m_bgm < 0) return;

	StopSoundMem(m_bgm);
	PlaySoundMem(m_bgm, loop ? DX_PLAYTYPE_LOOP : DX_PLAYTYPE_BACK);
}

void Audio::PlayBgm(const std::string& path, bool loop)
{
	if (LoadBgm(path)) {
		PlayBgm(loop);
	}
}

void Audio::StopBgm()
{
	if (m_bgm < 0) return;
	StopSoundMem(m_bgm);
}