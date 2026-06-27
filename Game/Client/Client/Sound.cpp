#include "pch.h"
#include "Sound.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sound

Sound::Sound(const std::string& strPath, bool bLoop, bool b3D, SoundCategory eCategory)
	: m_bLoop(bLoop)
	, m_b3D(b3D)
	, m_eCategory(eCategory)
{
	FMOD_MODE eFlag = b3D ? FMOD_3D : FMOD_2D;
	eFlag |= bLoop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;

	FMOD_System_CreateSound(SoundManager::m_gpSoundSystem, strPath.c_str(), eFlag, 0, &m_pSound);

	if (m_pSound && b3D) {
		FMOD_Sound_Set3DMinMaxDistance(m_pSound, SOUND_3D_MIN_DISTANCE, SOUND_3D_MAX_DISTANCE);
	}
}

Sound::~Sound()
{
	if (m_pSound) {
		FMOD_Sound_Release(m_pSound);
	}
}
