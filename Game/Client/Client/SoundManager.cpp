#include "pch.h"
#include "SoundManager.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sound

Sound::Sound(const std::string& strPath, bool bLoop)
{
	FMOD_MODE eFlag = bLoop ? FMOD_LOOP_NORMAL : FMOD_DEFAULT;
	FMOD_System_CreateSound(SoundManager::m_gpSoundSystem, strPath.c_str(), eFlag, 0, &m_pSound);

	m_bLoop = bLoop;
}

Sound::~Sound()
{
	FMOD_Sound_Release(m_pSound);
}

void Sound::Update()
{
	FMOD_Channel_IsPlaying(m_pChannel, &m_bLoop);

	if (m_bLoop) {
		FMOD_System_Update(SoundManager::m_gpSoundSystem);
	}
}

void Sound::Play()
{
	FMOD_System_PlaySound(SoundManager::m_gpSoundSystem, m_pSound, NULL, false, &m_pChannel);
}

void Sound::Pause() const
{
	FMOD_Channel_SetPaused(m_pChannel, true);
}

void Sound::Resume() const
{
	FMOD_Channel_SetPaused(m_pChannel, false);
}

void Sound::Stop() const
{
	FMOD_Channel_Stop(m_pChannel);
}

void Sound::VolumeUp()
{
	if (m_fVolume < SOUND_MAX) {
		m_fVolume += SOUND_WEIGHT;
	}

	FMOD_Channel_SetVolume(m_pChannel, m_fVolume);
}

void Sound::VolumeDown()
{
	if (m_fVolume > SOUND_MIN) {
		m_fVolume -= SOUND_WEIGHT;
	}

	FMOD_Channel_SetVolume(m_pChannel, m_fVolume);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SoundManager

FMOD_SYSTEM* SoundManager::m_gpSoundSystem = nullptr;

SoundManager::SoundManager()
{
}

SoundManager::~SoundManager()
{
	m_pSoundMap.clear();

	FMOD_System_Close(m_gpSoundSystem);
	FMOD_System_Release(m_gpSoundSystem);
}

void SoundManager::LoadSounds()
{
	AddSound("bgm", "../Resource/sound/Gravitation_2018_Remix.mp3", true);
	AddSound("laser_effect", "../Resource/sound/laser_sound.mp3", false);
	AddSound("explosion_effect", "../Resource/sound/explosion_sound.mp3", false);
	VolumeDown("explosion_effect");
	VolumeDown("explosion_effect");
	AddSound("spaceship_sfx", "../Resource/sound/spaceship-ambient-sfx.mp3", true);
	Play("spaceship_sfx");
	Pause("spaceship_sfx");

	AddSound("damage_sound", "../Resource/sound/damage_sound.mp3", false);

	AddSound("ending", "../Resource/sound/Stellardrone_-_Pale_Blue_Dot.mp3", true);
	VolumeUp("ending");
	VolumeUp("ending");
	VolumeUp("ending");
}

void SoundManager::Initialize()
{
	FMOD_System_Create(&m_gpSoundSystem, FMOD_VERSION);
	FMOD_System_Init(m_gpSoundSystem, 32, FMOD_INIT_NORMAL, NULL);

	LoadSounds();
}

void SoundManager::Update()
{
	for (const auto& [key, sound] : m_pSoundMap) {
		sound->Update();
	}
}

void SoundManager::AddSound(const std::string& strName, const std::string& strPath, bool bLoop)
{
	m_pSoundMap.insert({ strName, std::make_unique<Sound>(strPath, bLoop) });
}

void SoundManager::Play(const std::string& strName)
{
	m_pSoundMap[strName]->Play();
}

void SoundManager::Pause(const std::string& strName)
{
	if (!CheckExisting(strName)) {
		return;
	}

	m_pSoundMap[strName]->Pause();
}

void SoundManager::Resume(const std::string& strName)
{
	if (!CheckExisting(strName)) {
		return;
	}

	m_pSoundMap[strName]->Resume();
}

void SoundManager::Stop(const std::string& strName)
{
	if (!CheckExisting(strName)) {
		return;
	}

	m_pSoundMap[strName]->Stop();
}

void SoundManager::VolumeUp(const std::string& strName)
{
	if (!CheckExisting(strName)) {
		return;
	}

	m_pSoundMap[strName]->VolumeUp();
}

void SoundManager::VolumeDown(const std::string& strName)
{
	if (!CheckExisting(strName)) {
		return;
	}

	m_pSoundMap[strName]->VolumeDown();
}

bool SoundManager::CheckExisting(const std::string& strName) const
{
	if (!m_pSoundMap.contains(strName)) {
#ifdef _DEBUG
		OutputDebugStringA(std::format("{} doesn't extist\n", strName).c_str());
#endif
		return false;
	}

	return true;
}
