#pragma once

#define SOUND_MAX 1.0f
#define SOUND_MIN 0.0f
#define SOUND_DEFAULT 0.5f
#define SOUND_WEIGHT 0.1f

class Sound {
	friend class SoundManager;

public:
	Sound(const std::string& strPath, bool bLoop);
	~Sound();

private:
	void Update();

private:
	void Play();
	void Pause() const;
	void Resume() const;
	void Stop() const;
	void VolumeUp();
	void VolumeDown();

private:
	FMOD_SOUND*		m_pSound = nullptr;
	FMOD_CHANNEL*	m_pChannel = nullptr;
	FMOD_BOOL		m_bLoop = false;

	float	m_fVolume = SOUND_DEFAULT;

};

class SoundManager {
public:
	SoundManager();
	~SoundManager();

public:
	void LoadSounds();

public:
	void Initialize();
	void Update();

public:
	void AddSound(const std::string& strName, const std::string& strPath, bool bLoop);
	void Play(const std::string& strName);
	void Pause(const std::string& strName);
	void Resume(const std::string& strName);
	void Stop(const std::string& strName);
	void VolumeUp(const std::string& strName);
	void VolumeDown(const std::string& strName);

private:
	bool CheckExisting(const std::string& strName) const;

private:
	std::unordered_map<std::string, std::unique_ptr<Sound>> m_pSoundMap;

public:
	static FMOD_SYSTEM* m_gpSoundSystem;
};

