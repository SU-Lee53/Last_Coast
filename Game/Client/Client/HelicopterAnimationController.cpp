#include "pch.h"
#include "HelicopterAnimationController.h"
#include "Skeleton.h"

// ──────────────────────────────────────────────────────────────────────────
// HelicopterAnimationStateMachine
// ──────────────────────────────────────────────────────────────────────────

void HelicopterAnimationStateMachine::InitializeStateGraph()
{
	std::shared_ptr<AnimationState> pFly = std::make_shared<AnimationState>();

	pFly->strName = "Fly";
	pFly->pAnimationToPlay = ANIMATION->Get("Gunship_Anim");
	pFly->eAnimationPlayType = ANIMATION_PLAY_LOOP;
	pFly->fnStateTransitionCallback = nullptr; // single state, no transition

	m_pCurrentState = pFly;
	m_pStates.push_back(pFly);
}

// ──────────────────────────────────────────────────────────────────────────
// HelicopterAnimationController
// ──────────────────────────────────────────────────────────────────────────

HelicopterAnimationController::HelicopterAnimationController(std::shared_ptr<IGameObject> pOwner)
	: AnimationController{ pOwner }
{
}

void HelicopterAnimationController::Initialize()
{
	m_pStateMachine = std::make_unique<HelicopterAnimationStateMachine>();
	m_pStateMachine->Initialize(m_wpOwner.lock(), m_fTotalTimeElapsed);

	// No montage — base Update() null-checks m_pAnimationMontage.

	int nBones = GetOwnerBones().size();
	m_mtxCachedLocalBoneTransforms.resize(nBones);
	m_mtxFinalBoneTransforms.resize(nBones);
	m_mtxFinalModelLocalTransforms.resize(nBones);

	AnimationController::Initialize();
}

void HelicopterAnimationController::ComputeAnimation()
{
	// Gunship_Anim 클립을 상태머신에서 샘플링한 포즈를 그대로 사용 (파일 재생 방식).
	const std::vector<Bone>& ownerBones = GetOwnerBones();
	const std::vector<AnimationKey>& basePose = m_pStateMachine->GetOutputPose();

	m_mtxCachedLocalBoneTransforms.resize(ownerBones.size());
	for (int i = 0; i < (int)basePose.size(); ++i)
		m_mtxCachedLocalBoneTransforms[i] = basePose[i].CreateSRT();
}

std::shared_ptr<IComponent> HelicopterAnimationController::Copy(std::shared_ptr<IGameObject> pNewOwner) const
{
	auto pClone = std::make_shared<HelicopterAnimationController>(pNewOwner);
	pClone->Initialize();
	return pClone;
}
