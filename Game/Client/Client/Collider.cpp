#include "pch.h"
#include "Collider.h"

ICollider::ICollider(std::shared_ptr<IGameObject> pOwner)
	: IComponent{ pOwner }
{
}

void ICollider::MergeOBB(std::shared_ptr<IGameObject> pObj)
{
	if (m_bInitialized) {
		return;
	}

	auto pMeshRenderer = pObj->GetComponent<MeshRenderer>();
	if (pMeshRenderer) {
		if (m_xmOBBOrigin.Center == Vector3(0, 0, 0) && m_xmOBBOrigin.Extents == Vector3(1, 1, 1)) {
			m_xmOBBOrigin = pMeshRenderer->GetOBBMerged();
		}
		else {
			// Get corner fron OBB to merge
			XMFLOAT3 pxmf3OBBPoints1[BoundingOrientedBox::CORNER_COUNT];
			BoundingOrientedBox xmOBBMesh = pMeshRenderer->GetOBBMerged();
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
	}

	for (const auto& pChild : pObj->GetChildren()) {
		MergeOBB(pChild);
	}
}

void ICollider::Initialize()
{
	auto& pOwner = m_wpOwner.lock();
	MergeOBB(pOwner);

	const Matrix& mtxWorld = m_wpOwner.lock()->GetWorldMatrix();
	m_xmOBBOrigin.Transform(m_xmOBBWorld, mtxWorld);

	m_bInitialized = true;
}

bool ICollider::IsInFrustum(const BoundingFrustum& xmFrustumInWorld) const
{
	return xmFrustumInWorld.Intersects(m_xmOBBWorld);
}

bool ICollider::CheckCollision(std::shared_ptr<ICollider> pOther) const
{
	return m_xmOBBWorld.Intersects(pOther->m_xmOBBWorld);
}

const BoundingBox ICollider::GetAABBFromOBBWorld() const
{
	BoundingBox xmRet;
	XMFLOAT3 pxmf3OBBCorners[BoundingOrientedBox::CORNER_COUNT];
	m_xmOBBWorld.GetCorners(pxmf3OBBCorners);
	BoundingBox::CreateFromPoints(xmRet, BoundingOrientedBox::CORNER_COUNT, pxmf3OBBCorners, sizeof(XMFLOAT3));
	return xmRet;
}

//////////////////////////////////////////////////////////////////////////////////////
// StaticCollider

StaticCollider::StaticCollider(std::shared_ptr<IGameObject> pOwner)
	:ICollider{ pOwner }
{
}

void StaticCollider::Update()
{
	// Do nothing
}

std::shared_ptr<IComponent> StaticCollider::Copy(std::shared_ptr<IGameObject> pNewOwner) const
{
	std::shared_ptr<StaticCollider> pClone = std::make_shared<StaticCollider>(pNewOwner);
	pClone->m_xmOBBOrigin = m_xmOBBOrigin;
	pClone->m_xmOBBWorld = m_xmOBBWorld;
	pClone->m_bInitialized = m_bInitialized;

	return pClone;
}

//////////////////////////////////////////////////////////////////////////////////////
// DynamicCollider

DynamicCollider::DynamicCollider(std::shared_ptr<IGameObject> pOwner)
	:ICollider{ pOwner }
{
}

void DynamicCollider::Update()
{
	const Matrix& mtxWorld = m_wpOwner.lock()->GetWorldMatrix();
	m_xmOBBOrigin.Transform(m_xmOBBWorld, mtxWorld);
}

std::shared_ptr<IComponent> DynamicCollider::Copy(std::shared_ptr<IGameObject> pNewOwner) const
{
	std::shared_ptr<DynamicCollider> pClone = std::make_shared<DynamicCollider>(pNewOwner);
	pClone->m_xmOBBOrigin = m_xmOBBOrigin;
	pClone->m_xmOBBWorld = m_xmOBBWorld;
	pClone->m_bInitialized = m_bInitialized;

	return pClone;
}

