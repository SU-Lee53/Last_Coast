#pragma once
#include "DynamicObject.h"

class HelicopterObject : public DynamicObject {
public:
	~HelicopterObject() override;

	void Initialize() override;
	void ProcessInput() override;
	void Update() override;
	void PostUpdate() override;
	void PlaySound(const std::string& strSoundName);

	// 로터 회전 on/off (예: 추락 후 잔해는 정지)
	void SetRotorActive(bool bActive) { m_bRotorActive = bActive; }

protected:
	std::shared_ptr<IGameObject> m_pMainRotorFrame = nullptr;
	bool m_bRotorActive = true;
	FMOD_CHANNEL* m_pSoundChannel = nullptr;

};

