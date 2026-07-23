#pragma once

#define SOUND_MAX 1.0f
#define SOUND_MIN 0.0f
#define SOUND_DEFAULT 0.5f
#define SOUND_WEIGHT 0.05f

#define SOUND_DISTANCE_FACTOR 100.0f
#define SOUND_3D_MIN_DISTANCE 100.0f		
#define SOUND_3D_MAX_DISTANCE 5000.0f		

class Sound;

enum class SoundCategory {
	BGM,
	SFX,
	UIS,
	Count
};

class SoundManager {

	DECLARE_SINGLE(SoundManager)
	~SoundManager();

public:
	void Initialize();
	void Update();
	void UpdateSoundQueue();
	void LoadSounds();

public:
	void AddSound(const std::string& strName, const std::string& strPath, bool bLoop = false, bool b3D = false, SoundCategory eCategory = SoundCategory::SFX);


	FMOD_CHANNEL* Play(const std::string& strName);
	FMOD_CHANNEL* PlayAt(const std::string& strName, const Vector3& v3Position);

	FMOD_CHANNEL* Play(const std::shared_ptr<Sound>& pSound);
	FMOD_CHANNEL* PlayAt(const std::shared_ptr<Sound>& pSound, const Vector3& v3Position);

	void QueueSoundAt(const std::shared_ptr<Sound>& pSound, float fTimeAfter, const Vector3& v3Position = Vector3::Zero);

	void Pause(FMOD_CHANNEL* pChannel) const;
	void Resume(FMOD_CHANNEL* pChannel) const;
	void Stop(FMOD_CHANNEL* pChannel) const;
	void SetChannelVolume(FMOD_CHANNEL* pChannel, float fVolume) const;
	bool IsPlaying(FMOD_CHANNEL* pChannel) const;

	void SetCategoryVolume(SoundCategory eCategory, float fVolume);
	float GetCategoryVolume(SoundCategory eCategory) const;
	void CategoryVolumeUp(SoundCategory eCategory);
	void CategoryVolumeDown(SoundCategory eCategory);

	void SetMasterVolume(float fVolume);
	float GetMasterVolume() const;

	void SetListenerAttributes(const Vector3& v3Position, const Vector3& v3Velocity, const Vector3& v3Forward, const Vector3& v3Up);

	std::shared_ptr<Sound> GetSound(const std::string& strName) const;

private:
	bool CheckExisting(const std::string& strName) const;
	FMOD_CHANNELGROUP* GetGroup(SoundCategory eCategory) const;
	FMOD_CHANNEL* PlayInternal(const std::string& strName, bool b3D, const Vector3& v3Position);
	FMOD_CHANNEL* PlayInternal(Sound* pSound, bool b3D, const Vector3& v3Position);

private:
	std::unordered_map<std::string, std::shared_ptr<Sound>> m_pSoundMap;

	FMOD_CHANNELGROUP*	m_pMasterGroup = nullptr;
	FMOD_CHANNELGROUP*	m_pCategoryGroups[static_cast<int>(SoundCategory::Count)] = {};
	float				m_fCategoryVolume[static_cast<int>(SoundCategory::Count)] = {};

private:
	struct QueuedSound {
		float fTimeQueued;
		float fTimePlayAfter;
		Vector3 v3PlayAt;
		std::shared_ptr<Sound> pSoundToPlay;

		QueuedSound(const std::shared_ptr<Sound>& pSound, const Vector3& v3At, float fQueued, float fAfter)
			: fTimeQueued{ fQueued }, fTimePlayAfter{ fAfter }, v3PlayAt{ v3At }, pSoundToPlay{ pSound } {
		}

		struct Comp {
			bool operator()(const QueuedSound& lhs, const QueuedSound& rhs) const noexcept {
				return lhs.fTimeQueued + lhs.fTimePlayAfter > rhs.fTimeQueued + rhs.fTimePlayAfter;

			}
		};

	};

	std::priority_queue<QueuedSound, std::vector<QueuedSound>, QueuedSound::Comp> m_SoundQueued;

public:
	static FMOD_SYSTEM* m_gpSoundSystem;
};
