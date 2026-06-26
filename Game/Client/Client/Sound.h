#pragma once

class Sound {
	friend class SoundManager;

public:
	Sound(const std::string& strPath, bool bLoop, bool b3D, SoundCategory eCategory);
	~Sound();

private:
	FMOD_SOUND* m_pSound = nullptr;
	bool			m_bLoop = false;
	bool			m_b3D = false;
	SoundCategory	m_eCategory = SoundCategory::SFX;
};


