#include "pch.h"
#include "Sound.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sound

Sound::Sound(const std::string& strPath, bool bLoop, bool b3D, SoundCategory eCategory, float fMinDistance, float fMaxDistance)
	: m_bLoop(bLoop)
	, m_b3D(b3D)
	, m_eCategory(eCategory)
{
	FMOD_MODE eFlag = b3D ? FMOD_3D : FMOD_2D;
	eFlag |= bLoop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;

	FMOD_RESULT eResult = FMOD_System_CreateSound(SoundManager::m_gpSoundSystem, strPath.c_str(), eFlag, 0, &m_pSound);

	// 실패해도 조용히 넘어가면 해당 사운드만 영원히 무음이 된다 (파일 없음/비표준 포맷 등)
	if (eResult != FMOD_OK || !m_pSound) {
		char szBuf[512];
		sprintf_s(szBuf, "[Sound] FMOD 로드 실패 (result=%d): %s\n", static_cast<int>(eResult), strPath.c_str());
		OutputDebugStringA(szBuf);
	}

	if (m_pSound && b3D) {
		FMOD_Sound_Set3DMinMaxDistance(m_pSound, fMinDistance, fMaxDistance);
	}
}

Sound::~Sound()
{
	if (m_pSound) {
		FMOD_Sound_Release(m_pSound);
	}
}
