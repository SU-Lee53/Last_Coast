#include "pch.h"
#include "AnimationController.h"

void AnimationController::Update()
{
	m_pStateMachine->Update();
	m_pStateMachine->ComputeAnimation();

}

const std::vector<Matrix> AnimationController::GetFinalOutput() const
{
	return m_pStateMachine->GetFinalMatrix();
}

double AnimationController::GetCurrentAnimationDuration() const
{
	return m_pStateMachine->GetCurrentAnimationState()->pAnimationToPlay->GetDuration();
}

void PlayerAnimationController::Initialize(std::shared_ptr<GameObject> pOwner)
{
	m_wpOwner = pOwner;
	m_pStateMachine = std::make_unique<PlayerAnimationStateMachine>();
	m_pStateMachine->Initialize(pOwner);
	m_finalBoneTransforms.reserve(MAX_BONE_TRANSFORMS);

}
