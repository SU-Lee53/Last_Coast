#include "pch.h"
#include "SoundManager.h"
#include "Sound.h"

namespace {
	FMOD_VECTOR ToFmod(const Vector3& v)
	{
		FMOD_VECTOR out{ v.x, v.y, v.z };
		return out;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SoundManager

FMOD_SYSTEM* SoundManager::m_gpSoundSystem = nullptr;

SoundManager::~SoundManager()
{
	m_pSoundMap.clear();

	for (FMOD_CHANNELGROUP* pGroup : m_pCategoryGroups) {
		if (pGroup) {
			FMOD_ChannelGroup_Release(pGroup);
		}
	}

	FMOD_System_Close(m_gpSoundSystem);
	FMOD_System_Release(m_gpSoundSystem);
}

void SoundManager::Initialize()
{
	FMOD_System_Create(&m_gpSoundSystem, FMOD_VERSION);
	FMOD_System_Init(m_gpSoundSystem, 64, FMOD_INIT_NORMAL, NULL);

	FMOD_System_Set3DSettings(m_gpSoundSystem, 1.0f, SOUND_DISTANCE_FACTOR, 1.0f);

	FMOD_System_GetMasterChannelGroup(m_gpSoundSystem, &m_pMasterGroup);

	const char* groupNames[] = { "BGM", "SFX", "UI" };
	for (int i = 0; i < static_cast<int>(SoundCategory::Count); ++i) {
		FMOD_System_CreateChannelGroup(m_gpSoundSystem, groupNames[i], &m_pCategoryGroups[i]);
		FMOD_ChannelGroup_AddGroup(m_pMasterGroup, m_pCategoryGroups[i], false, nullptr);

		m_fCategoryVolume[i] = SOUND_DEFAULT;
		FMOD_ChannelGroup_SetVolume(m_pCategoryGroups[i], SOUND_DEFAULT);
	}

	LoadSounds();
}

void SoundManager::Update()
{
	FMOD_System_Update(m_gpSoundSystem);
}

void SoundManager::UpdateSoundQueue()
{
	auto fnCheck = [](const QueuedSound& queued) -> bool {
		return TIME->GetTotalTime() >= queued.fTimeQueued + queued.fTimePlayAfter;
	};

	while (!m_SoundQueued.empty()) {
		const auto& curQueued = m_SoundQueued.top();
		if (fnCheck(curQueued)) {
			if (curQueued.v3PlayAt == Vector3::Zero) {
				Play(curQueued.pSoundToPlay);
			}
			else {
				PlayAt(curQueued.pSoundToPlay, curQueued.v3PlayAt);
			}

			m_SoundQueued.pop();
		}
		else {
			break;
		}
	}
}

void SoundManager::LoadSounds()
{
	AddSound("Test", "../Resources/Sounds/Test.wav", false, false, SoundCategory::SFX);
	AddSound("Test3D", "../Resources/Sounds/Test.wav", false, true, SoundCategory::SFX);
}

void SoundManager::AddSound(const std::string& strName, const std::string& strPath, bool bLoop, bool b3D, SoundCategory eCategory, float fMinDistance, float fMaxDistance)
{
	if (m_pSoundMap.contains(strName)) {
		return;
	}

	m_pSoundMap.insert({ strName, std::make_shared<Sound>(strPath, bLoop, b3D, eCategory, fMinDistance, fMaxDistance) });
}

FMOD_CHANNEL* SoundManager::PlayInternal(const std::string& strName, bool b3D, const Vector3& v3Position)
{
	if (!CheckExisting(strName)) {
		return nullptr;
	}

	return PlayInternal(m_pSoundMap[strName].get(), b3D, v3Position);
}

FMOD_CHANNEL* SoundManager::PlayInternal(Sound* pSound, bool b3D, const Vector3& v3Position)
{
	if (!pSound) {
		return nullptr;
	}

	FMOD_CHANNELGROUP* pGroup = GetGroup(pSound->m_eCategory);

	// Start paused so 3D attributes are applied before the first audible sample.
	FMOD_CHANNEL* pChannel = nullptr;
	FMOD_System_PlaySound(m_gpSoundSystem, pSound->m_pSound, pGroup, true, &pChannel);

	if (pChannel && b3D && pSound->m_b3D) {
		FMOD_VECTOR pos = ToFmod(v3Position);
		FMOD_VECTOR vel{ 0.0f, 0.0f, 0.0f };
		FMOD_Channel_Set3DAttributes(pChannel, &pos, &vel);
	}

	if (pChannel) {
		FMOD_Channel_SetPaused(pChannel, false);
	}

	return pChannel;
}

FMOD_CHANNEL* SoundManager::Play(const std::string& strName)
{
	return PlayInternal(strName, false, Vector3::Zero);
}

FMOD_CHANNEL* SoundManager::PlayAt(const std::string& strName, const Vector3& v3Position)
{
	return PlayInternal(strName, true, v3Position);
}

FMOD_CHANNEL* SoundManager::Play(const std::shared_ptr<Sound>& pSound)
{
	return pSound ? PlayInternal(pSound.get(), false, Vector3::Zero) : nullptr;
}

FMOD_CHANNEL* SoundManager::PlayAt(const std::shared_ptr<Sound>& pSound, const Vector3& v3Position)
{
	return pSound ? PlayInternal(pSound.get(), true, v3Position) : nullptr;
}
 
void SoundManager::QueueSoundAt(const std::shared_ptr<Sound>& pSound, float fTimeAfter, const Vector3& v3Position)
{
	m_SoundQueued.emplace(pSound, v3Position, TIME->GetTotalTime(), fTimeAfter);
}

void SoundManager::Pause(FMOD_CHANNEL* pChannel) const
{
	if (pChannel) {
		FMOD_Channel_SetPaused(pChannel, true);
	}
}

void SoundManager::Resume(FMOD_CHANNEL* pChannel) const
{
	if (pChannel) {
		FMOD_Channel_SetPaused(pChannel, false);
	}
}

void SoundManager::Stop(FMOD_CHANNEL* pChannel) const
{
	if (pChannel) {
		FMOD_Channel_Stop(pChannel);
	}
}

void SoundManager::SetChannelVolume(FMOD_CHANNEL* pChannel, float fVolume) const
{
	if (pChannel) {
		FMOD_Channel_SetVolume(pChannel, std::clamp(fVolume, SOUND_MIN, SOUND_MAX));
	}
}

void SoundManager::SetChannelPosition(FMOD_CHANNEL* pChannel, const Vector3& v3Position) const
{
	if (pChannel) {
		FMOD_VECTOR v3FmodPosition = ToFmod(v3Position);
		FMOD_VECTOR v3FmodVelocity{};
		FMOD_Channel_Set3DAttributes(pChannel, &v3FmodPosition, &v3FmodVelocity);
	}
}

bool SoundManager::IsPlaying(FMOD_CHANNEL* pChannel) const
{
	if (!pChannel) {
		return false;
	}

	FMOD_BOOL bPlaying = false;
	if (FMOD_Channel_IsPlaying(pChannel, &bPlaying) != FMOD_OK) {
		return false;
	}

	return bPlaying != 0;
}

void SoundManager::StopCategory(SoundCategory eCategory) const
{
	if (FMOD_CHANNELGROUP* pGroup = GetGroup(eCategory)) {
		FMOD_ChannelGroup_Stop(pGroup);
	}
}

void SoundManager::SetCategoryMute(SoundCategory eCategory, bool bMute) const
{
	if (FMOD_CHANNELGROUP* pGroup = GetGroup(eCategory)) {
		FMOD_ChannelGroup_SetMute(pGroup, bMute);
	}
}

void SoundManager::SetCategoryVolume(SoundCategory eCategory, float fVolume)
{
	fVolume = std::clamp(fVolume, SOUND_MIN, SOUND_MAX);

	int nIndex = static_cast<int>(eCategory);
	m_fCategoryVolume[nIndex] = fVolume;
	FMOD_ChannelGroup_SetVolume(m_pCategoryGroups[nIndex], fVolume);
}

float SoundManager::GetCategoryVolume(SoundCategory eCategory) const
{
	return m_fCategoryVolume[static_cast<int>(eCategory)];
}

void SoundManager::CategoryVolumeUp(SoundCategory eCategory)
{
	SetCategoryVolume(eCategory, GetCategoryVolume(eCategory) + SOUND_WEIGHT);
}

void SoundManager::CategoryVolumeDown(SoundCategory eCategory)
{
	SetCategoryVolume(eCategory, GetCategoryVolume(eCategory) - SOUND_WEIGHT);
}

void SoundManager::SetMasterVolume(float fVolume)
{
	fVolume = std::clamp(fVolume, SOUND_MIN, SOUND_MAX);
	FMOD_ChannelGroup_SetVolume(m_pMasterGroup, fVolume);
}

float SoundManager::GetMasterVolume() const
{
	float fVolume = SOUND_DEFAULT;
	FMOD_ChannelGroup_GetVolume(m_pMasterGroup, &fVolume);
	return fVolume;
}

void SoundManager::SetListenerAttributes(const Vector3& v3Position, const Vector3& v3Velocity, const Vector3& v3Forward, const Vector3& v3Up)
{
	FMOD_VECTOR pos = ToFmod(v3Position);
	FMOD_VECTOR vel = ToFmod(v3Velocity);
	FMOD_VECTOR forward = ToFmod(v3Forward);
	FMOD_VECTOR up = ToFmod(v3Up);

	FMOD_System_Set3DListenerAttributes(m_gpSoundSystem, 0, &pos, &vel, &forward, &up);
}

std::shared_ptr<Sound> SoundManager::GetSound(const std::string& strName) const
{
	auto it = m_pSoundMap.find(strName);
	return (it != m_pSoundMap.end()) ? it->second : nullptr;
}

bool SoundManager::CheckExisting(const std::string& strName) const
{
	if (!m_pSoundMap.contains(strName)) {
#ifdef _DEBUG
		OutputDebugStringA(std::format("{} doesn't exist\n", strName).c_str());
#endif
		return false;
	}

	return true;
}

FMOD_CHANNELGROUP* SoundManager::GetGroup(SoundCategory eCategory) const
{
	return m_pCategoryGroups[static_cast<int>(eCategory)];
}
