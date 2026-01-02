#include "pch.h"
#include "AnimationController.h"

void AnimationController::Update()
{
	m_dTimeElapsed += DT * 100;	// Test
	m_pStateMachine->Update();

	const std::vector<Bone>& bones = m_wpOwner.lock()->GetBones();
	size_t nBones = bones.size();
	std::vector<Matrix> mtxToParentTransforms;	// reserve, resize 필요 X -> 아래 ComputeAnimation 에서 resize함
	m_pStateMachine->ComputeAnimation(mtxToParentTransforms, m_dTimeElapsed);

	// TODO : boneTransforms 를 이용한 최종 애니메이션 변환 행렬을 구함
	// 1. 정점을 자신의 부모 본 기준으로 하는 변환 행렬이 필요 -> GetSRT가 부모 기준 변환을 다루므로 이미 구함
	// 2. 정점을 Root로 보내는 변환 행렬이 필요

	std::vector<Matrix> mtxToRootTransforms(nBones);
	for (int i = 0; i < nBones; ++i) {
		int nParentIndex = bones[i].nParentIndex;
		Matrix mtxToRoot = nParentIndex >= 0 ? mtxToParentTransforms[i] * mtxToRootTransforms[nParentIndex] : mtxToParentTransforms[i];
		mtxToRootTransforms[i] = mtxToRoot;
	}

	m_finalBoneTransforms.resize(nBones);
	for (int i = 0; i < nBones; ++i) {
		m_finalBoneTransforms[i] = bones[i].mtxOffset * mtxToRootTransforms[i];
		m_finalBoneTransforms[i] = m_finalBoneTransforms[i].Transpose();
	}
}

const std::vector<Matrix> AnimationController::GetKeyframeSRT() const
{
	return m_finalBoneTransforms;
}

double AnimationController::GetCurrentAnimationDuration() const
{
	return m_pStateMachine->GetCurrentAnimationState()->pAnimationToPlay->GetDuration();;
}

void PlayerAnimationController::Initialize(std::shared_ptr<GameObject> pOwner)
{
	m_wpOwner = pOwner;
	m_pStateMachine = std::make_unique<PlayerAnimationStateMachine>();
	m_pStateMachine->Initialize(pOwner);
	m_finalBoneTransforms.reserve(pOwner->GetBones().size());

}
