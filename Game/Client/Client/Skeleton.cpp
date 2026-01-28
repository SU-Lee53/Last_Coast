#include "pch.h"
#include "Skeleton.h"

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
}

void Skeleton::Update()
{
}

std::shared_ptr<IComponent> Skeleton::Copy(std::shared_ptr<IGameObject> pNewOwner)const
{
	std::shared_ptr<Skeleton> pClone = std::make_shared<Skeleton>(pNewOwner);
	pClone->m_Bones = m_Bones;
	pClone->m_nRootBoneIndex = m_nRootBoneIndex;
	pClone->m_bInitialized = true;

    return pClone;
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
