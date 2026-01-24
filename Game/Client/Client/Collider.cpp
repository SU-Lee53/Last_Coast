#include "pch.h"
#include "Collider.h"

Collider::Collider(std::shared_ptr<GameObject> pOwner)
	: IComponent{ pOwner }
{
}

void Collider::MergeOBB(std::shared_ptr<GameObject> pObj)
{
	if (m_bInitialized) {
		return;
	}

	auto pMeshRenderer = pObj->GetComponent<MeshRenderer>();
	if (pMeshRenderer) {
		// Get corner fron OBB to merge
		XMFLOAT3 pxmf3OBBPoints1[BoundingOrientedBox::CORNER_COUNT];
		BoundingOrientedBox xmOBBMesh;
		pMeshRenderer->GetOBBMerged().Transform(xmOBBMesh, pObj->GetTransform()->GetWorldMatrix());
		xmOBBMesh.GetCorners(pxmf3OBBPoints1);

		XMFLOAT3 pxmf3OBBPoints2[BoundingOrientedBox::CORNER_COUNT];
		m_xmOBBOrigin.GetCorners(pxmf3OBBPoints2);

		// Create AABB from OBB points for merge
		BoundingBox xmAABB1, xmAABB2;
		BoundingBox::CreateFromPoints(xmAABB1, BoundingOrientedBox::CORNER_COUNT, pxmf3OBBPoints1, sizeof(XMFLOAT3));
		BoundingBox::CreateFromPoints(xmAABB2, BoundingOrientedBox::CORNER_COUNT, pxmf3OBBPoints2, sizeof(XMFLOAT3));

		// Merge OBB
		BoundingBox xmAABBMerged;
		BoundingBox::CreateMerged(xmAABBMerged, xmAABB1, xmAABB2);

		// Set OBB
		BoundingOrientedBox::CreateFromBoundingBox(m_xmOBBOrigin, xmAABBMerged);
	}

	for (const auto& pChild : pObj->GetChildren()) {
		MergeOBB(pChild);
	}
}

void Collider::Initialize()
{
	auto& pOwner = m_wpOwner.lock();
	MergeOBB(pOwner);

	const Matrix& mtxWorld = m_wpOwner.lock()->GetWorldMatrix();
	m_xmOBBOrigin.Transform(m_xmOBBWorld, mtxWorld);

	m_bInitialized = true;
}

bool Collider::IsInFrustum(const BoundingFrustum& xmFrustumInWorld)
{
	return xmFrustumInWorld.Intersects(m_xmOBBWorld);
}

//////////////////////////////////////////////////////////////////////////////////////
// StaticCollider

StaticCollider::StaticCollider(std::shared_ptr<GameObject> pOwner)
	:Collider{ pOwner }
{
}

void StaticCollider::Update()
{
	// Do nothing
}

std::shared_ptr<IComponent> StaticCollider::Copy(std::shared_ptr<GameObject> pNewOwner)
{
	std::shared_ptr<StaticCollider> pClone = std::make_shared<StaticCollider>(pNewOwner);
	pClone->m_xmOBBOrigin = m_xmOBBOrigin;
	pClone->m_xmOBBWorld = m_xmOBBWorld;
	pClone->m_bInitialized = m_bInitialized;

	return pClone;
}

//////////////////////////////////////////////////////////////////////////////////////
// DynamicCollider

DynamicCollider::DynamicCollider(std::shared_ptr<GameObject> pOwner)
	:Collider{ pOwner }
{
}

void DynamicCollider::Update()
{
	const Matrix& mtxWorld = m_wpOwner.lock()->GetWorldMatrix();
	m_xmOBBOrigin.Transform(m_xmOBBWorld, mtxWorld);
}

std::shared_ptr<IComponent> DynamicCollider::Copy(std::shared_ptr<GameObject> pNewOwner)
{
	std::shared_ptr<DynamicCollider> pClone = std::make_shared<DynamicCollider>(pNewOwner);
	pClone->m_xmOBBOrigin = m_xmOBBOrigin;
	pClone->m_xmOBBWorld = m_xmOBBWorld;
	pClone->m_bInitialized = m_bInitialized;

	return pClone;
}

