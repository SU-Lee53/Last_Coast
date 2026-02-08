#pragma once
#include "DynamicObject.h"

class Camera;

interface IPlayer abstract : public DynamicObject {
public:
	IPlayer();
	virtual ~IPlayer();

	virtual void PostUpdate() override;

public:
	virtual ClientToServerPacket MakePacketToSend() { return ClientToServerPacket{}; }

public:
	const std::shared_ptr<Camera>& GetCamera() const { return m_pCamera; };

protected:
	std::shared_ptr<Camera> m_pCamera = nullptr;

};

