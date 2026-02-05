#include "pch.h"
#include "Defines.h"
#include "GameObject.h"


float CollisionResult::GetDistanceBetweenCenter() const 
{
	auto [col1, col2] = DecomposeRef();
	return Vector3::Distance(col1.m_xmOBBWorld.Center, col2.m_xmOBBWorld.Center);
}

Vector3 CollisionResult::GetDirectionToOther() const 
{
	auto [col1, col2] = DecomposeRef();
	return XMVector3Normalize(XMVectorSubtract(XMLoadFloat3(&col2.m_xmOBBWorld.Center), XMLoadFloat3(&col2.m_xmOBBWorld.Center)));
}

std::pair<const ICollider&, const ICollider&> CollisionResult::DecomposeRef() const 
{
	const auto& pCollider1 = pSelf->GetComponent<ICollider>();
	const auto& pCollider2 = pOther->GetComponent<ICollider>();
	return { *pCollider1, *pCollider2 };
}
