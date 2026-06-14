#pragma once

#define SOUND_MAX 1.0f
#define SOUND_MIN 0.0f
#define SOUND_DEFAULT 0.5f
#define SOUND_WEIGHT 0.1f


#define SOUND_DISTANCE_FACTOR 100.0f
#define SOUND_3D_MIN_DISTANCE 100.0f		
#define SOUND_3D_MAX_DISTANCE 5000.0f		

enum class SoundCategory {
	BGM,
	SFX,
	UIS,
	Count
};

class Sound {
	friend class SoundManager;

public:
	Sound(const std::string& strPath, bool bLoop, bool b3D, SoundCategory eCategory);
	~Sound();

private:
	FMOD_SOUND*		m_pSound = nullptr;
	bool			m_bLoop = false;
	bool			m_b3D = false;
	SoundCategory	m_eCategory = SoundCategory::SFX;
};

class SoundManager {

	DECLARE_SINGLE(SoundManager)
	~SoundManager();

public:
	void Initialize();
	void Update();
	void LoadSounds();

public:
	void AddSound(const std::string& strName, const std::string& strPath, bool bLoop = false, bool b3D = false, SoundCategory eCategory = SoundCategory::SFX);


	FMOD_CHANNEL* Play(const std::string& strName);
	FMOD_CHANNEL* PlayAt(const std::string& strName, const Vector3& v3Position);

	void Pause(FMOD_CHANNEL* pChannel) const;
	void Resume(FMOD_CHANNEL* pChannel) const;
	void Stop(FMOD_CHANNEL* pChannel) const;
	bool IsPlaying(FMOD_CHANNEL* pChannel) const;

	void SetCategoryVolume(SoundCategory eCategory, float fVolume);
	float GetCategoryVolume(SoundCategory eCategory) const;
	void CategoryVolumeUp(SoundCategory eCategory);
	void CategoryVolumeDown(SoundCategory eCategory);

	void SetMasterVolume(float fVolume);
	float GetMasterVolume() const;

	void SetListenerAttributes(const Vector3& v3Position, const Vector3& v3Velocity, const Vector3& v3Forward, const Vector3& v3Up);

private:
	bool CheckExisting(const std::string& strName) const;
	FMOD_CHANNELGROUP* GetGroup(SoundCategory eCategory) const;
	FMOD_CHANNEL* PlayInternal(const std::string& strName, bool b3D, const Vector3& v3Position);

private:
	std::unordered_map<std::string, std::unique_ptr<Sound>> m_pSoundMap;

	FMOD_CHANNELGROUP*	m_pMasterGroup = nullptr;
	FMOD_CHANNELGROUP*	m_pCategoryGroups[static_cast<int>(SoundCategory::Count)] = {};
	float				m_fCategoryVolume[static_cast<int>(SoundCategory::Count)] = {};

public:
	static FMOD_SYSTEM* m_gpSoundSystem;
};
