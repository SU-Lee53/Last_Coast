#include "pch.h"
#include "GameObject.h"
#include "Transform.h"

GameObject::GameObject()
{
}

GameObject::~GameObject()
{
}

void GameObject::Initialize()
{
	if (!m_bInitialized) {
		for (auto& component : m_pComponents) {
			if (component) {
				component->Initialize();
			}
		}

		if (m_pMeshRenderer) {
			m_pMeshRenderer->Initialize();
		}

		if (m_pAnimationController) {
			m_pAnimationController->Initialize(shared_from_this());
		}
	}

	for (auto& pChild : m_pChildren) {
		pChild->Initialize();
	}

	if (m_pParent.expired()) {
		MergeBoundingBox(&m_xmOBB);
	}

	m_bInitialized = true;
}

void GameObject::Update()
{
	for (auto& component : m_pComponents) {
		if (component) {
			component->Update();
		}
	}

	m_Transform.Update(m_pParent.lock());

	if (m_pAnimationController) {
		m_pAnimationController->Update();
	}

	for (auto& pChild : m_pChildren) {
		pChild->Update();
	}
}

void GameObject::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList)
{
	// TODO : Render Logic Here
	if (m_pAnimationController) {
		RENDER->AddAnimatedObject(shared_from_this());
		return;
	}

	if (m_pMeshRenderer) {
		m_pMeshRenderer->Update(shared_from_this());
	}

	for (auto& pChild : m_pChildren) {
		pChild->Render(pd3dCommandList);
	}
}

void GameObject::RenderDirectly(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& descHandle)
{
	if (m_pMeshRenderer) {
		int nInstanceBase = -1;
		Matrix mtxWorld = m_Transform.GetWorldMatrix();
		m_pMeshRenderer->Render(pd3dDevice, pd3dCommandList, descHandle, 1, nInstanceBase, mtxWorld);
	}

	for (auto& pChild : m_pChildren) {
		pChild->RenderDirectly(pd3dDevice, pd3dCommandList, descHandle);
	}
}

void GameObject::SetBound(const Vector3& v3Center, const Vector3& v3Extents)
{
	m_xmOBB.Center = v3Center;
	m_xmOBB.Extents = v3Extents;
}

void GameObject::SetParent(std::shared_ptr<GameObject> pParent)
{
	if (pParent) {
		m_pParent = pParent;
	}
}

void GameObject::SetChild(std::shared_ptr<GameObject> pChild)
{
	if (pChild)
	{
		pChild->m_pParent = shared_from_this();
		m_pChildren.push_back(pChild);
	}

	// 현재 프레임이 Root 이고 추가될 자식이 애니메이션을 가지고 있다면 옮겨온다
	if (m_pParent.expired() && pChild->m_pAnimationController) {
		m_Bones = std::move(pChild->m_Bones);
		m_pAnimationController = std::move(pChild->m_pAnimationController);
	}

}

void GameObject::SetFrameName(const std::string& strFrameName)
{
	m_strFrameName = strFrameName;
}

int GameObject::FindBoneIndex(const std::string& strBoneName) const
{
	if (m_Bones.size() == 0) {
		return -1;
	}
	
	std::vector<const Bone*> DFSStack;
	DFSStack.reserve(m_Bones.size());
	DFSStack.push_back(&m_Bones[m_nRootBoneIndex]);

	const Bone* pCurBone = nullptr;
	while (true) {
		if (DFSStack.size() == 0) {
			break;
		}

		pCurBone = DFSStack.back();
		DFSStack.pop_back();

		if (pCurBone->strBoneName == strBoneName) {
			return pCurBone->nIndex;
		}

		for (int i = 0; i < pCurBone->nChildren; ++i) {
			DFSStack.push_back(&m_Bones[pCurBone->nChilerenIndex[i]]);
		}
	}

	return -1;
}

void GameObject::MergeBoundingBox(BoundingOrientedBox* pOBB)
{
	if (m_pParent.expired()) {	// if root
		*pOBB = BoundingOrientedBox{};
	}

	if (m_pMeshRenderer) {
		// Get corner fron OBB to merge
		XMFLOAT3 pxmf3OBBPoints1[BoundingOrientedBox::CORNER_COUNT];
		m_pMeshRenderer->GetOBBMerged().GetCorners(pxmf3OBBPoints1);
		
		XMFLOAT3 pxmf3OBBPoints2[BoundingOrientedBox::CORNER_COUNT];
		pOBB->GetCorners(pxmf3OBBPoints2);

		// Create AABB from obb points for merge
		BoundingBox xmAABB1, xmAABB2;
		BoundingBox::CreateFromPoints(xmAABB1, BoundingOrientedBox::CORNER_COUNT, pxmf3OBBPoints1, sizeof(XMFLOAT3));
		BoundingBox::CreateFromPoints(xmAABB2, BoundingOrientedBox::CORNER_COUNT, pxmf3OBBPoints2, sizeof(XMFLOAT3));

		// Merge OBB
		BoundingBox xmAABBMerged;
		BoundingBox::CreateMerged(xmAABBMerged, xmAABB1, xmAABB2);

		// Set OBB
		BoundingOrientedBox::CreateFromBoundingBox(*pOBB, xmAABBMerged);
	}

	for (auto& pChild : m_pChildren) {
		pChild->MergeBoundingBox(pOBB);
	}
}

std::shared_ptr<GameObject> GameObject::FindFrame(const std::string& strFrameName)
{
	std::shared_ptr<GameObject> pFrameObject;
	if (strFrameName == m_strFrameName) {
		return shared_from_this();
	}

	for (auto& pChild : m_pChildren) {
		if (pFrameObject = pChild->FindFrame(strFrameName)) {
			return pFrameObject;
		}
	}

	return nullptr;
}

std::shared_ptr<GameObject> GameObject::FindMeshedFrame(const std::string& strFrameName)
{
	std::shared_ptr<GameObject> pFrameObject;
	if (strFrameName == m_strFrameName && m_pMeshRenderer) {
		return shared_from_this();
	}

	for (auto& pChild : m_pChildren) {
		if (pFrameObject = pChild->FindMeshedFrame(strFrameName)) {
			return pFrameObject;
		}
	}

	return nullptr;
}
