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

	auto pSkeleton = pOwner->GetComponent<Skeleton>();
	if (pSkeleton) {
		const std::vector<Bone>& ownerBones = pSkeleton->GetBones();
		for (auto& pState : m_pStates) {
			if (!pState || !pState->pAnimationToPlay) {
				continue;
			}

			pState->channelIndices.resize(ownerBones.size(), INVALID_ID);
			for (const auto& bone : ownerBones) {
				pState->channelIndices[bone.nIndex] =
					pState->pAnimationToPlay->GetChannelIndex(bone.strBoneName);
			}
		}
	}

	m_fLastAnimationChangedTime = m_fTotalAnimationTime;
	m_fCurrentTransitionTime = 0.0f;
	m_fCurrentAnimationStartOffset = 0.0f;
	m_fBeforeAnimationTimeAtTransition = 0.0f;
}

void AnimationStateMachine::Update()
{
	auto pOwner = m_wpOwner.lock();
	if (!pOwner || !m_pCurrentState || !m_pCurrentState->pAnimationToPlay) {
		return;
	}

	m_fTotalAnimationTime += DT;

	// State transition check
	std::shared_ptr<AnimationState> pNextState = nullptr;
	float fNextTransitionTime = 0.0f;

	for (const auto& edge : m_pCurrentState->pConnectedEdges) {
		auto pConnectedState = edge.pConnectedState.lock();
		if (!pConnectedState) {
			continue;
		}

		if (!pConnectedState->fnStateTransitionCallback) {
			continue;
		}

		if (pConnectedState->fnStateTransitionCallback(pOwner)) {
			pNextState = pConnectedState;
			fNextTransitionTime = static_cast<float>(edge.dTransitionTime);
			break;
		}
	}

	if (pNextState && pNextState != m_pCurrentState) {
		const float fPrevStateElapsed = m_fTotalAnimationTime - m_fLastAnimationChangedTime;

		m_fBeforeAnimationTimeAtTransition = m_pCurrentState->pAnimationToPlay->GetLoopedAnimationTime(m_fCurrentAnimationStartOffset + fPrevStateElapsed);

		m_pBeforeState = m_pCurrentState;
		m_pCurrentState = pNextState;

		m_fCurrentTransitionTime = fNextTransitionTime;
		m_fLastAnimationChangedTime = m_fTotalAnimationTime;

		// Play new animation from first frame
		m_fCurrentAnimationStartOffset = 0.0f;
	}

	auto pSkeleton = pOwner->GetComponent<Skeleton>();
	if (!pSkeleton) {
		return;
	}

	const std::vector<Bone>& ownerBones = pSkeleton->GetBones();
	const int nBones = static_cast<int>(ownerBones.size());

	auto pCurrentAnimation = m_pCurrentState->pAnimationToPlay;
	if (!pCurrentAnimation) {
		return;
	}

	m_OutputPose.resize(nBones);

	const float fStateElapsed = m_fTotalAnimationTime - m_fLastAnimationChangedTime;
	const float fCurrentAnimTime = pCurrentAnimation->GetLoopedAnimationTime(m_fCurrentAnimationStartOffset + fStateElapsed);

	if (m_pBeforeState &&
		m_pBeforeState->pAnimationToPlay &&
		fStateElapsed < m_fCurrentTransitionTime &&
		m_fCurrentTransitionTime > 0.0f) {

		auto pBeforeAnimation = m_pBeforeState->pAnimationToPlay;

		const float fBeforeAnimTime =
			pBeforeAnimation->GetLoopedAnimationTime(
				m_fBeforeAnimationTimeAtTransition + fStateElapsed);

		float fWeight = std::clamp(fStateElapsed / m_fCurrentTransitionTime, 0.0f, 1.0f);
		fWeight = ::SmoothStep(fWeight, 0.0f, 1.0f);

		for (const auto& bone : ownerBones) {
			AnimationKey key0 =
				pBeforeAnimation->GetKeyFrameSRT(
					m_pBeforeState->channelIndices[bone.nIndex],
					fBeforeAnimTime,
					bone.mtxTransform
				);

			AnimationKey key1 =
				pCurrentAnimation->GetKeyFrameSRT(
					m_pCurrentState->channelIndices[bone.nIndex],
					fCurrentAnimTime,
					bone.mtxTransform
				);

			m_OutputPose[bone.nIndex] = AnimationKey::Lerp(key0, key1, fWeight);
		}
	}
	else {
		m_pBeforeState = nullptr;

		for (const auto& bone : ownerBones) {
			m_OutputPose[bone.nIndex] =
				pCurrentAnimation->GetKeyFrameSRT(
					m_pCurrentState->channelIndices[bone.nIndex],
					fCurrentAnimTime,
					bone.mtxTransform
				);
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
	pIdle->Connect(pRun, 0.2);

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
	
	auto pPlayer = std::static_pointer_cast<IThirdPersonPlayer>(pObj);
	return pPlayer->GetMoveSpeedSqXZ() < std::numeric_limits<float>::epsilon();
	//return !pPlayer->IsMoving();
}

bool PlayerAnimationStateMachine::WalkCallback(std::shared_ptr<IGameObject> pObj)
{
	// TODO : 구현
	//return INPUT->GetButtonPressed(VK_UP) && !INPUT->GetButtonPressed(VK_LSHIFT);
	
	auto pPlayer = std::static_pointer_cast<IThirdPersonPlayer>(pObj);
	return pPlayer->GetMoveSpeedSqXZ() > std::numeric_limits<float>::epsilon() && !pPlayer->IsRunning();
	//return pPlayer->IsMoving() && !pPlayer->IsRunning();
}

bool PlayerAnimationStateMachine::RunCallback(std::shared_ptr<IGameObject> pObj)
{
	// TODO : 구현
	// return INPUT->GetButtonPressed(VK_UP) && INPUT->GetButtonPressed(VK_LSHIFT);
	
	auto pPlayer = std::static_pointer_cast<IThirdPersonPlayer>(pObj);
	return pPlayer->GetMoveSpeedSqXZ() > std::numeric_limits<float>::epsilon() && pPlayer->IsRunning();
	//return pPlayer->IsMoving() && pPlayer->IsRunning();
}
