#include "pch.h"
#include "AnimationStateMachine.h"
#include "ThirdPersonPlayer.h"

AnimationStateMachine::AnimationStateMachine()
{
	m_OutputPose.reserve(MAX_BONE_TRANSFORMS);
}

void AnimationStateMachine::Initialize(std::shared_ptr<IGameObject> pOwner, float fInitialTime)
{
	m_wpOwner = pOwner;
	m_fTotalAnimationTime = fInitialTime;

	InitializeStateGraph();
}

void AnimationStateMachine::Update()
{
	m_fTotalAnimationTime += DT;			// Test

	// Update StateMachine
	std::shared_ptr<AnimationState> pNextState = nullptr;
	for (const auto& pEdges : m_pCurrentState->pConnectedEdges) {
		if (!pEdges.pConnectedState.expired()) {
			if (pEdges.pConnectedState.lock()->fnStateTransitionCallback(m_wpOwner.lock())) {
				pNextState = pEdges.pConnectedState.lock();
				m_fCurrentTransitionTime = pEdges.dTransitionTime;
				break;
			}
		}
	}

	if (pNextState) {
		m_pBeforeState = m_pCurrentState;
		m_pCurrentState = pNextState;

		m_fLastAnimationChangedTime = m_fTotalAnimationTime;
	}

	const std::vector<Bone>& ownerBones = m_wpOwner.lock()->GetComponent<Skeleton>()->GetBones();
	int nBones = ownerBones.size();
	auto pAnimation = m_pCurrentState->pAnimationToPlay;

	float fCurrentTime = m_fTotalAnimationTime - m_fLastAnimationChangedTime;
	float fTime = std::fmod(m_fTotalAnimationTime, pAnimation->GetDuration());
	
	m_OutputPose.resize(nBones);
	if (fCurrentTime < m_fCurrentTransitionTime) {
		auto pLastAnimation = m_pBeforeState->pAnimationToPlay;
		float fLastTime = std::fmod(m_fLastAnimationChangedTime + fCurrentTime, pLastAnimation->GetDuration());

		float fWeight = std::clamp(fCurrentTime / m_fCurrentTransitionTime, 0.f, 1.f);
		fWeight = ::SmoothStep(fWeight, 0.f, 1.f);

		for (const auto& bone : ownerBones) {
			AnimationKey key0 = pLastAnimation->GetKeyFrameSRT(bone.strBoneName, fLastTime, bone.mtxTransform);
			AnimationKey key1 = pAnimation->GetKeyFrameSRT(bone.strBoneName, fTime, bone.mtxTransform);
			m_OutputPose[bone.nIndex] = AnimationKey::Lerp(key0, key1, fWeight);
		}
	}
	else {
		for (const auto& bone : ownerBones) {
			m_OutputPose[bone.nIndex] = pAnimation->GetKeyFrameSRT(bone.strBoneName, fTime, bone.mtxTransform);
		}
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
	pIdle->pAnimationToPlay = ANIMATION->Get("Breathing Idle");
	//pIdle->pAnimationToPlay = ANIMATION->Get("Silly Dancing");
	pIdle->eAnimationPlayType = ANIMATION_PLAY_LOOP;
	pIdle->fnStateTransitionCallback = IdleCallback;
	
	pWalk->strName = "Walk";
	pWalk->pAnimationToPlay = ANIMATION->Get("Walking");
	pWalk->eAnimationPlayType = ANIMATION_PLAY_LOOP;
	pWalk->fnStateTransitionCallback = WalkCallback;
	
	pRun->strName = "Run";
	pRun->pAnimationToPlay = ANIMATION->Get("Jog Forward");
	pRun->eAnimationPlayType = ANIMATION_PLAY_LOOP;
	pRun->fnStateTransitionCallback = RunCallback;

	pIdle->Connect(pWalk, 0.2);

	pWalk->Connect(pIdle, 0.2);
	pWalk->Connect(pRun, 0.2);

	pRun->Connect(pWalk, 0.2);
	pRun->Connect(pIdle, 0.2);

	m_pCurrentState = pIdle;

	m_pStates.push_back(pIdle);
	m_pStates.push_back(pWalk);
	m_pStates.push_back(pRun);

}

bool PlayerAnimationStateMachine::IdleCallback(std::shared_ptr<IGameObject> pObj)
{
	// TODO : 구현
	//return !INPUT->GetButtonPressed(VK_UP) && !INPUT->GetButtonPressed(VK_LSHIFT);
	
	auto pPlayer = std::static_pointer_cast<ThirdPersonPlayer>(pObj);
	return pPlayer->GetMoveSpeed() < std::numeric_limits<float>::epsilon();
}

bool PlayerAnimationStateMachine::WalkCallback(std::shared_ptr<IGameObject> pObj)
{
	// TODO : 구현
	//return INPUT->GetButtonPressed(VK_UP) && !INPUT->GetButtonPressed(VK_LSHIFT);
	
	auto pPlayer = std::static_pointer_cast<ThirdPersonPlayer>(pObj);
	return pPlayer->GetMoveSpeed() > std::numeric_limits<float>::epsilon() && !pPlayer->IsRunning();
}

bool PlayerAnimationStateMachine::RunCallback(std::shared_ptr<IGameObject> pObj)
{
	// TODO : 구현
	// return INPUT->GetButtonPressed(VK_UP) && INPUT->GetButtonPressed(VK_LSHIFT);
	
	auto pPlayer = std::static_pointer_cast<ThirdPersonPlayer>(pObj);
	return pPlayer->GetMoveSpeed() > std::numeric_limits<float>::epsilon() && pPlayer->IsRunning();
}
