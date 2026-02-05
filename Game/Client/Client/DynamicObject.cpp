#include "pch.h"
#include "DynamicObject.h"


void DynamicObject::PreUpdate()
{
}

void DynamicObject::PostUpdate()
{
	for (auto& component : m_pComponents) {
		if (component) {
			component->Update();
		}
	}

	for (auto& pChild : m_pChildren) {
		pChild->PostUpdate();
	}
}
