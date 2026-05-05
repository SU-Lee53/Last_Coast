#include "pch.h"
#include "Skeleton.h"
#include "WeaponObject.h"

WeaponSocket::WeaponSocket(std::shared_ptr<Skeleton> pOwner, int32 nBoneIndex)
	: IAttachSocket{ pOwner, nBoneIndex } 
{
	m_pWeaponModel = std::make_shared<WeaponObject>();
}

void WeaponSocket::Initialize()
{
	m_pWeaponModel->Initialize();
}

void WeaponSocket::Update()
{
	m_pWeaponModel->Update();

	if (m_pWeaponModel) {
		auto pSkeleton = m_wpOwner.lock();

		Matrix mtxAnimation = Matrix::Identity;
		if (const auto& mtxModelLocals = pSkeleton->TryGetAnimationModelLocalTransforms()) {
			mtxAnimation = (*mtxModelLocals)[m_nAttachedBoneIndex];
			mtxAnimation = mtxAnimation * Matrix::CreateRotationY(XMConvertToRadians(-90.f));
		}

		auto pPlayer = pSkeleton->GetOwner();
		const auto& mtxPlayerWorld = pPlayer->GetWorldMatrix();
		m_mtxTransform = m_pWeaponModel->GetOffsetTransform() * mtxAnimation * mtxPlayerWorld;

		m_pWeaponModel->GetTransform()->SetWorldMatrix(m_mtxTransform);
		m_pWeaponModel->UpdateMuzzlePositionWorld(m_mtxTransform);

		m_pWeaponModel->PostUpdate();
	}
}

void WeaponSocket::Render()
{
	m_pWeaponModel->Render();
}

bool WeaponSocket::TryFire()
{
	if (!m_pWeaponModel) return false;
	return m_pWeaponModel->TryFire();
}

void WeaponSocket::SetWeapon(WEAPON_TYPE eWeaponType)
{
	m_pWeaponModel = static_pointer_cast<WeaponObject>(GCTX->GetWeaponCopy(eWeaponType));
	m_eCurrentWeapon = eWeaponType;
}

Skeleton::Skeleton(std::shared_ptr<IGameObject> pOwner)
	: IComponent{ pOwner }
{
}

Skeleton::Skeleton(std::shared_ptr<IGameObject> pOwner, std::vector<Bone>& bones)
	: IComponent{ pOwner }
{
	m_Bones = std::move(bones);
}

void Skeleton::Initialize()
{
	for (const auto& bone : m_Bones | std::views::filter([](const Bone& b) { return b.nParentIndex == -1; })) {
		m_nRootBoneIndex = bone.nIndex;
	}

	for (auto& pAttached: m_pAttached) {
		pAttached->Initialize();
	}
}

void Skeleton::Update()
{
	if (auto pAnim = m_wpOwner.lock()->GetComponent<AnimationController>()) {
		m_pFinalModelLocalRef = &(pAnim->GetFinalModelLocalTransforms());
	}

	for (auto& pSocket : m_pAttached) {
		pSocket->Update();
	}
}

std::shared_ptr<IComponent> Skeleton::Copy(std::shared_ptr<IGameObject> pNewOwner)const
{
	std::shared_ptr<Skeleton> pClone = std::make_shared<Skeleton>(pNewOwner);
	pClone->m_Bones = m_Bones;
	pClone->m_nRootBoneIndex = m_nRootBoneIndex;
	pClone->m_bInitialized = true;
	//pClone->ResetBoneNode(pNewOwner);

    return pClone;
}

void Skeleton::PrepareRenderAttached()
{
	for (auto& pObj : m_pAttached) {
		if (pObj) {
			pObj->Render();
		}
	}
}

int Skeleton::FindBoneIndex(const std::string& strBoneName) const
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

void Skeleton::DetachSocket(std::shared_ptr<IAttachSocket> pAttachable)
{
	std::erase(m_pAttached, pAttachable);
}

void Skeleton::ShowControlImGui()
{
	ImGui::SeparatorText("Attach Sockets");
	for (auto& pSocket : m_pAttached) {
		ImGuiHelper::PrintMatrix("Transform Matrix", pSocket->m_mtxTransform);
		ImGuiHelper::PrintTransformMatrix("Transform Matrix Decomposed", pSocket->m_mtxTransform);

		if (auto p = std::dynamic_pointer_cast<WeaponSocket>(pSocket)) {
			if (auto& pModel = p->GetWeaponModel()) {
				// Edit offset
				pModel->EditStat();

				
				// Save offset
				if (ImGui::Button("Save")) {
					pModel->SaveStat();
				}
				
				// Show model info
				pModel->ShowControlImGui();
			}
		}

	}

	ImGui::SeparatorText("Bones");
	if (ImGui::TreeNode("Open Bones")) {
		for (const auto& bone : m_Bones) {
			if (ImGui::TreeNode(bone.strBoneName.c_str())) {
				ImGui::Text("bone Name : %s", bone.strBoneName.c_str());
				ImGui::Text("bone Index : %d", bone.nIndex);
				ImGui::Text("bone Depth : %d", bone.nDepth);
				ImGuiHelper::PrintTransformMatrix("Offset Matrix", bone.mtxOffset);
				ImGuiHelper::PrintTransformMatrix("Transform Matrix", bone.mtxTransform);

				ImGui::Text("Parent Index: %d", bone.nParentIndex);
				ImGui::Text("NumChildren: %d", bone.nChildren);
				ImGui::Indent(20.f);
				ImGui::Text("Children Index: ");
				for (int n : bone.nChilerenIndex) {
					ImGui::SameLine();
					ImGui::Text(" %d", n);
				}
				ImGui::Unindent(20.f);

				bone.pNode->ShowControlImGui();

				ImGui::TreePop();
			}
		}

		ImGui::TreePop();
	}
}

void Skeleton::ResetBoneNodes()
{
	auto pOwner = m_wpOwner.lock();
	for (auto& bone : m_Bones) {
		bone.pNode = pOwner->FindFrameEndsWith(bone.strBoneName);
	}
}
