#pragma once
#include "GameObject.h"

class Camera;

class Player : public GameObject {
public:
	Player();
	virtual ~Player();

public:
	virtual void Initialize() override;
	virtual void ProcessInput() override;
	virtual void Update() override;
	virtual void Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList) override;

	virtual ClientToServerPacket MakePacketToSend() { return ClientToServerPacket{}; }

public:
	const std::shared_ptr<Camera>& GetCamera() const { return m_pCamera; };

protected:
	std::shared_ptr<Camera> m_pCamera = nullptr;

};

