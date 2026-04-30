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
	if (m_pWeaponModel) {
		auto pSkeleton = m_wpOwner.lock();

		Matrix mtxAnimation = Matrix::Identity;
		if (const auto& mtxModelLocals = pSkeleton->TryGetAnimationModelLocalTransforms()) {
			mtxAnimation = (*mtxModelLocals)[m_nAttachedBoneIndex];
			mtxAnimation = mtxAnimation * Matrix::CreateRotationY(XMConvertToRadians(-90.f));
		}

		float fYaw = XMConvertToRadians(m_v3OffsetRotation.y);
		float fPitch = XMConvertToRadians(m_v3OffsetRotation.x);
		float fRoll = XMConvertToRadians(m_v3OffsetRotation.z);
		Matrix mtxOffset = Matrix::CreateFromYawPitchRoll(fYaw, fPitch, fRoll);
		mtxOffset.Translation(m_v3OffsetPosition);

		auto pPlayer = pSkeleton->GetOwner();
		const auto& mtxPlayerWorld = pPlayer->GetWorldMatrix();
		m_mtxTransform = mtxOffset * mtxAnimation * mtxPlayerWorld;

		m_pWeaponModel->GetTransform()->SetWorldMatrix(m_mtxTransform);
		m_pWeaponModel->PostUpdate();
	}
}

void WeaponSocket::Render()
{
	m_pWeaponModel->Render();
}

void WeaponSocket::SetWeapon(WEAPON_TYPE eWeaponType)
{
	m_pWeaponModel->SetWeapon(eWeaponType);

	m_v3OffsetPosition = GCTX->GetWeaponOffsetPosition(eWeaponType);
	m_v3OffsetRotation = GCTX->GetWeaponOffsetRotation(eWeaponType);
}

void WeaponSocket::EditOffset() 
{
	ImGui::DragFloat3("Offset Position", reinterpret_cast<float*>(&m_v3OffsetPosition), 0.1f);
	ImGui::DragFloat3("Offset Rotation", reinterpret_cast<float*>(&m_v3OffsetRotation), 0.1f);
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
				p->EditOffset();
				
				// Save offset
				if (ImGui::Button("Save")) {
					std::string strSavePath = "../Resources/Scenes/Weapons.json";
					if (!std::filesystem::exists(strSavePath)) {
						std::filesystem::create_directories(strSavePath);
					}
					std::ifstream in;
					in.open(strSavePath, std::ios::ate);

					nlohmann::json j;
					if (in.tellg() > 0) {
						in.seekg(0, std::ios::beg);
						j = nlohmann::json::parse(in);
					}

					std::string strWeaponName{ GameContext::g_cstrWeaponName[static_cast<size_t>(pModel->GetWeaponType())] };
					
					const Vector3& v3OffsetPos = p->GetOffsetPosition();
					const Vector3& v3OffsetRotation = p->GetOffsetRotation();
					j[strWeaponName]["OffsetPosition"] = { v3OffsetPos.x, v3OffsetPos.y, v3OffsetPos.z };
					j[strWeaponName]["OffsetRotation"] = { v3OffsetRotation.x, v3OffsetRotation.y, v3OffsetRotation.z };

					// Save json for readability 
					std::ofstream out(strSavePath, std::ios::trunc);
					if (!out) {
						__debugbreak();
						return;
					}

					out << j.dump(4) << '\n';
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
