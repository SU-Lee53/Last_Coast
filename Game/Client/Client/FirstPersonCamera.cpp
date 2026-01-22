#include "pch.h"
#include "FirstPersonCamera.h"

void FirstPersonCamera::Update()
{
	Vector3 v3PlayerLook = m_wpOwner.lock()->GetTransform()->GetLook();
	SetLookTo(v3PlayerLook);
	
	Vector3 v3PlayerPosition = m_wpOwner.lock()->GetTransform()->GetPosition();
	SetPosition(v3PlayerPosition);

	Camera::Update();
}
