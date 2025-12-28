#include "pch.h"
#include "Component.h"
#include "GameObject.h"

Component::Component(std::shared_ptr<GameObject> pOwner)
	: m_wpOwner{ pOwner }
{
}
