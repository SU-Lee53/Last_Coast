#include "pch.h"
#include "AnimationStateMachine.h"

void AnimationStateMachine::Initialize(std::shared_ptr<GameObject> pOwner)
{
	m_wpOwner = pOwner;

	InitializeStateGraph();
}

void AnimationStateMachine::Update()
{
	std::shared_ptr<AnimationState> pNextState = nullptr;
	for (const auto& pEdges : m_pCurrentState->pConnectedEdges) {
		if (!pEdges.pConnectedState.expired()) {
			if (pEdges.pConnectedState.lock()->fnStateTransitionCallback(m_wpOwner.lock())) {
				pNextState = pEdges.pConnectedState.lock();
				break;
			}
		}
	}

	if (pNextState) {
		m_pCurrentState = pNextState;
	}

}

void AnimationStateMachine::ComputeAnimation(std::vector<Matrix>& boneTransforms, double dTimeElapsed) const
{
	const std::vector<Bone>& ownerBones = m_wpOwner.lock()->GetBones();
	auto pAnimation = m_pCurrentState->pAnimationToPlay;

	double dTime = fmod(dTimeElapsed, pAnimation->GetDuration());
	boneTransforms.resize(ownerBones.size());
	for (const auto& bone : ownerBones) {
		boneTransforms[bone.nIndex] = pAnimation->GetSRT(bone.strBoneName, dTime, bone.mtxTransform);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PlayerAnimationStateMachine

void PlayerAnimationStateMachine::InitializeStateGraph()
{
	std::shared_ptr<AnimationState> pIdle = std::make_shared<AnimationState>();
	std::shared_ptr<AnimationState> pWalk = std::make_shared<AnimationState>();
	std::shared_ptr<AnimationState> pRun = std::make_shared<AnimationState>();
	
	pIdle->strName = "Idle";
	//pIdle->pAnimationToPlay = ANIMATION->Get("Breathing Idle");
	pIdle->pAnimationToPlay = ANIMATION->Get("Silly Dancing");
	pIdle->eAnimationPlayType = ANIMATION_PLAY_LOOP;
	pIdle->fnStateTransitionCallback = IdleCallback;
	
	pWalk->strName = "Walk";
	pWalk->pAnimationToPlay = ANIMATION->Get("Walk");
	pWalk->eAnimationPlayType = ANIMATION_PLAY_LOOP;
	pWalk->fnStateTransitionCallback = WalkCallback;
	
	pRun->strName = "Run";
	pRun->pAnimationToPlay = ANIMATION->Get("Run");
	pRun->eAnimationPlayType = ANIMATION_PLAY_LOOP;
	pRun->fnStateTransitionCallback = RunCallback;

	pIdle->Connect(pWalk, 0.5);

	pWalk->Connect(pIdle, 0.5);
	pWalk->Connect(pRun, 0.2);

	pRun->Connect(pWalk, 0.2);
	pRun->Connect(pIdle, 0.5);

	m_pCurrentState = pIdle;

	m_pStates.push_back(pIdle);
	m_pStates.push_back(pWalk);
	m_pStates.push_back(pRun);

}

bool PlayerAnimationStateMachine::IdleCallback(std::shared_ptr<GameObject> pObj)
{
	// TODO : 구현
	return false;
}

bool PlayerAnimationStateMachine::WalkCallback(std::shared_ptr<GameObject> pObj)
{
	// TODO : 구현
	return false;
}

bool PlayerAnimationStateMachine::RunCallback(std::shared_ptr<GameObject> pObj)
{
	// TODO : 구현
	return false;
}
