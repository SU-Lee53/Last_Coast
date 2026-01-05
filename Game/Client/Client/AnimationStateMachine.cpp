#include "pch.h"
#include "AnimationStateMachine.h"

AnimationStateMachine::AnimationStateMachine()
{
	m_mtxfinalBoneTransforms.reserve(MAX_BONE_TRANSFORMS);
}

void AnimationStateMachine::Initialize(std::shared_ptr<GameObject> pOwner)
{
	m_wpOwner = pOwner;

	InitializeStateGraph();
}

void AnimationStateMachine::Update()
{
	m_dTotalTimeElapsed += DT * 20;			// Test
	m_dCurrentAnimationTime += DT * 20;		// Test

	// Update StateMachine
	std::shared_ptr<AnimationState> pNextState = nullptr;
	for (const auto& pEdges : m_pCurrentState->pConnectedEdges) {
		if (!pEdges.pConnectedState.expired()) {
			if (pEdges.pConnectedState.lock()->fnStateTransitionCallback(m_wpOwner.lock())) {
				pNextState = pEdges.pConnectedState.lock();
				m_dCurrentTransitionTime = pEdges.dTransitionTime;
				break;
			}
		}
	}

	if (pNextState) {
		m_pBeforeState = m_pCurrentState;
		m_pCurrentState = pNextState;

		m_dLastAnimationChangedTime = m_dTotalTimeElapsed;
		m_dCurrentAnimationTime = 0.f;
	}
}

void AnimationStateMachine::ComputeAnimation()
{
	const std::vector<Bone>& ownerBones = m_wpOwner.lock()->GetBones();
	int nBones = ownerBones.size();
	auto pAnimation = m_pCurrentState->pAnimationToPlay;

	double dTime = std::fmod(m_dCurrentAnimationTime, pAnimation->GetDuration());

	std::vector<Matrix> boneTransforms(ownerBones.size());

	if (m_dCurrentAnimationTime < m_dCurrentTransitionTime) {
		auto pLastAnimation = m_pBeforeState->pAnimationToPlay;
		double dLastTime = std::fmod(m_dLastAnimationChangedTime, pLastAnimation->GetDuration());
		double dWeight = std::clamp((m_dTotalTimeElapsed - m_dLastAnimationChangedTime) / m_dCurrentTransitionTime, 0.0, 1.0);
		dWeight = ::SmoothStep(dWeight, 0.0, 1.0);

		for (const auto& bone : ownerBones) {
			AnimationKey key0 = pLastAnimation->GetKeyFrameSRT(bone.strBoneName, m_dLastAnimationChangedTime + dTime, bone.mtxTransform);
			AnimationKey key1 = pAnimation->GetKeyFrameSRT(bone.strBoneName, dTime, bone.mtxTransform);
			boneTransforms[bone.nIndex] = AnimationKey::Lerp(key0, key1, dWeight).CreateSRT();
		}
	}
	else {
		for (const auto& bone : ownerBones) {
			boneTransforms[bone.nIndex] = pAnimation->GetKeyFrameMatrix(bone.strBoneName, dTime, bone.mtxTransform);
		}
	}

	std::vector<Matrix> mtxToRootTransforms(nBones);
	for (int i = 0; i < nBones; ++i) {
		int nParentIndex = ownerBones[i].nParentIndex;
		Matrix mtxToRoot = nParentIndex >= 0 ? boneTransforms[i] * mtxToRootTransforms[nParentIndex] : boneTransforms[i];
		mtxToRootTransforms[i] = mtxToRoot;
	}

	m_mtxfinalBoneTransforms.resize(nBones);
	for (int i = 0; i < nBones; ++i) {
		m_mtxfinalBoneTransforms[i] = ownerBones[i].mtxOffset * mtxToRootTransforms[i];
		m_mtxfinalBoneTransforms[i] = m_mtxfinalBoneTransforms[i].Transpose();
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

	pIdle->Connect(pWalk, 1.0);

	pWalk->Connect(pIdle, 1.0);
	pWalk->Connect(pRun, 1.0);

	pRun->Connect(pWalk, 1.0);
	pRun->Connect(pIdle, 1.0);

	m_pCurrentState = pIdle;

	m_pStates.push_back(pIdle);
	m_pStates.push_back(pWalk);
	m_pStates.push_back(pRun);

}

bool PlayerAnimationStateMachine::IdleCallback(std::shared_ptr<GameObject> pObj)
{
	// TODO : 구현
	return !INPUT->GetButtonPressed(VK_UP) && !INPUT->GetButtonPressed(VK_LSHIFT);
}

bool PlayerAnimationStateMachine::WalkCallback(std::shared_ptr<GameObject> pObj)
{
	// TODO : 구현

	return INPUT->GetButtonPressed(VK_UP) && !INPUT->GetButtonPressed(VK_LSHIFT);
}

bool PlayerAnimationStateMachine::RunCallback(std::shared_ptr<GameObject> pObj)
{
	// TODO : 구현
	return INPUT->GetButtonPressed(VK_UP) && INPUT->GetButtonPressed(VK_LSHIFT);
}
