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

	// 2025.11.10
	// MakePacketToSend() By 이승욱
	// Player Base 클래스에서는 선언만 함 
	// 정의는 override 받은 SpaceshipPlayer에서 함
	virtual ClientToServerPacket MakePacketToSend() { return ClientToServerPacket{}; }

public:
	const std::shared_ptr<Camera>& GetCamera() const { return m_pCamera; };

protected:
	std::shared_ptr<Camera> m_pCamera = nullptr;

};

