#include "pch.h"
#include "ZombieAnimationController.h"
#include "Zombie.h"
#include "Skeleton.h"

// ──────────────────────────────────────────────────────────────────────────
// ZombieAnimationStateMachine
// ──────────────────────────────────────────────────────────────────────────

void ZombieAnimationStateMachine::InitializeStateGraph()
{
	std::shared_ptr<AnimationState> pIdle = std::make_shared<AnimationState>();
	std::shared_ptr<AnimationState> pWalk = std::make_shared<AnimationState>();

	pIdle->strName = "Idle";
	pIdle->pAnimationToPlay = ANIMATION->Get("Breathing Idle");
	pIdle->eAnimationPlayType = ANIMATION_PLAY_LOOP;
	pIdle->fnStateTransitionCallback = IdleCallback;

	pWalk->strName = "Walk";
	pWalk->pAnimationToPlay = ANIMATION->Get("Walking");
	pWalk->eAnimationPlayType = ANIMATION_PLAY_LOOP;
	pWalk->fnStateTransitionCallback = WalkCallback;

	pIdle->Connect(pWalk, 0.2);
	pWalk->Connect(pIdle, 0.2);

	m_pCurrentState = pIdle;

	m_pStates.push_back(pIdle);
	m_pStates.push_back(pWalk);
}

bool ZombieAnimationStateMachine::IdleCallback(std::shared_ptr<IGameObject> pObj)
{
	auto pZombie = std::static_pointer_cast<Zombie>(pObj);
	return pZombie->GetMoveSpeedSqXZ() < std::numeric_limits<float>::epsilon();
}

bool ZombieAnimationStateMachine::WalkCallback(std::shared_ptr<IGameObject> pObj)
{
	auto pZombie = std::static_pointer_cast<Zombie>(pObj);
	return pZombie->GetMoveSpeedSqXZ() >= std::numeric_limits<float>::epsilon();
}

// ──────────────────────────────────────────────────────────────────────────
// ZombieAnimationController
// ──────────────────────────────────────────────────────────────────────────

ZombieAnimationController::ZombieAnimationController(std::shared_ptr<IGameObject> pOwner)
	: AnimationController{ pOwner }
{
}

void ZombieAnimationController::Initialize()
{
	m_pStateMachine = std::make_unique<ZombieAnimationStateMachine>();
	m_pStateMachine->Initialize(m_wpOwner.lock(), m_fTotalTimeElapsed);

	m_pAnimationMontage = std::make_unique<ZombieAnimationMontage>();
	m_pAnimationMontage->Initialize(m_wpOwner.lock());

	int nBones = GetOwnerBones().size();
	m_mtxCachedLocalBoneTransforms.resize(nBones);
	m_mtxFinalBoneTransforms.resize(nBones);
}

void ZombieAnimationController::ComputeAnimation()
{
	const std::vector<Bone>& ownerBones = GetOwnerBones();
	const std::vector<AnimationKey>& basePose = m_pStateMachine->GetOutputPose();
	float fMontageWeight = m_pAnimationMontage->GetBlendWeight();

	m_mtxCachedLocalBoneTransforms.resize(ownerBones.size());
	if (fMontageWeight > 0.f) {
		const std::vector<AnimationKey>& montagePose = m_pAnimationMontage->GetOutputPose();
		for (int i = 0; i < (int)ownerBones.size(); ++i)
			m_mtxCachedLocalBoneTransforms[i] = AnimationKey::Lerp(basePose[i], montagePose[i], fMontageWeight).CreateSRT();
	}
	else {
		for (int i = 0; i < (int)basePose.size(); ++i)
			m_mtxCachedLocalBoneTransforms[i] = basePose[i].CreateSRT();
	}
}

std::shared_ptr<IComponent> ZombieAnimationController::Copy(std::shared_ptr<IGameObject> pNewOwner) const
{
	auto pClone = std::make_shared<ZombieAnimationController>(pNewOwner);
	pClone->Initialize();
	return pClone;
}
