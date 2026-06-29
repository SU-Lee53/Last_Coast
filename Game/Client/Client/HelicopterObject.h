#pragma once
#include "DynamicObject.h"

class HelicopterObject : public DynamicObject {
public:
	void Initialize() override;
	void ProcessInput() override;
	void Update() override;

	// 로터 회전 on/off (예: 추락 후 잔해는 정지)
	void SetRotorActive(bool bActive) { m_bRotorActive = bActive; }

protected:
	std::shared_ptr<IGameObject> m_pMainRotorFrame = nullptr;
	bool m_bRotorActive = true;

};

