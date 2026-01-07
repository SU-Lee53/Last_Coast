#include "pch.h"
#include "AnimationController.h"

LayeredBlendMachine::LayeredBlendMachine(std::shared_ptr<GameObject> pGameObject, const std::string& strBranch)
	: strBranchBoneName{ strBranch }
{
	const auto& ownerBones = pGameObject->GetBones();
	int nBranchIndex = pGameObject->FindBoneIndex(strBranch);
	nBones = ownerBones.size();
	bLayerMask.resize(nBones);

	// 상하체 Mask 생성
	std::vector<const Bone*> DFSStack;
	DFSStack.reserve(ownerBones.size());
	DFSStack.push_back(&ownerBones[nBranchIndex]);

	int nBlendRootDepth = ownerBones[nBranchIndex].nDepth;
	const Bone* pCurBone = nullptr;
	while (true) {
		if (DFSStack.size() == 0) {
			break;
		}

		pCurBone = DFSStack.back();
		DFSStack.pop_back();

		bLayerMask[pCurBone->nIndex] = TRUE;

		for (int i = 0; i < pCurBone->nChildren; ++i) {
			DFSStack.push_back(&ownerBones[pCurBone->nChilerenIndex[i]]);
		}
	}
}

void LayeredBlendMachine::Blend(const std::vector<AnimationKey>& mtxBasePose, const std::vector<AnimationKey>& mtxBlendPose, std::vector<AnimationKey>& outOutputPose) const
{
	outOutputPose.resize(nBones);

	for (int i = 0; i < nBones; ++i) {
		outOutputPose[i] = bLayerMask[i] ? mtxBlendPose[i] : mtxBasePose[i];
	}
}

void AnimationController::Update()
{
	m_fTotalTimeElapsed += DT;
	if (m_pStateMachine) {
		m_pStateMachine->Update();
	}

	ComputeAnimation();
	ComputeFinalMatrix();
}

const std::vector<Matrix> AnimationController::GetFinalOutput() const
{
	return m_mtxFinalBoneTransforms;
}

double AnimationController::GetCurrentAnimationDuration() const
{
	return m_pStateMachine->GetCurrentAnimationState()->pAnimationToPlay->GetDuration();
}

void AnimationController::ComputeFinalMatrix()
{
	const std::vector<Bone>& ownerBones = m_wpOwner.lock()->GetBones();
	int nBones = ownerBones.size();
	std::vector<Matrix> mtxToRootTransforms(nBones);
	for (int i = 0; i < nBones; ++i) {
		int nParentIndex = ownerBones[i].nParentIndex;
		Matrix mtxToRoot = nParentIndex >= 0 ? m_mtxCachedBoneTransforms[i] * mtxToRootTransforms[nParentIndex] : m_mtxCachedBoneTransforms[i];
		mtxToRootTransforms[i] = mtxToRoot;
	}

	m_mtxFinalBoneTransforms.resize(nBones);
	for (int i = 0; i < nBones; ++i) {
		m_mtxFinalBoneTransforms[i] = ownerBones[i].mtxOffset * mtxToRootTransforms[i];
		m_mtxFinalBoneTransforms[i] = m_mtxFinalBoneTransforms[i].Transpose();
	}
}

void AnimationController::LayerdBlendPerBone(const std::vector<AnimationKey>& mtxPose1, const std::vector<AnimationKey>& mtxPose2, const std::string& strBranchBoneName, float fBlendWeight, int nBlendDepth)
{
	// mtxPose1 과 mtxPose2 를 섞는다
	// 우선 상체 / 하체의 분리가 먼저 필요
}

const std::vector<Bone>& AnimationController::GetOwnerBones() const
{
	return m_wpOwner.lock()->GetBones();
}

void AnimationController::CacheAnimatioKey(const std::string& strAnimationName)
{
	const auto& ownerBones = GetOwnerBones();
	m_mtxCachedPose.resize(ownerBones.size());
	const auto& pAnimation = ANIMATION->Get(strAnimationName);
	double fTime = std::fmod(m_fTotalTimeElapsed, pAnimation->GetDuration());

	for (const auto& bone : ownerBones) {
		m_mtxCachedPose[bone.nIndex] = pAnimation->GetKeyFrameSRT(bone.strBoneName, fTime, bone.mtxTransform);
	}
}

void PlayerAnimationController::Initialize(std::shared_ptr<GameObject> pOwner)
{
	m_wpOwner = pOwner;
	m_pStateMachine = std::make_unique<PlayerAnimationStateMachine>();
	m_pStateMachine->Initialize(pOwner, m_fTotalTimeElapsed);
	m_mtxFinalBoneTransforms.resize(MAX_BONE_TRANSFORMS);
	m_mtxCachedBoneTransforms.resize(MAX_BONE_TRANSFORMS);

	m_pBlendMachine = std::make_unique<LayeredBlendMachine>(pOwner, "Spine");
}

void PlayerAnimationController::ComputeAnimation()
{
	const std::vector<Bone>& ownerBones = GetOwnerBones();
	int n = m_wpOwner.lock()->FindBoneIndex("Spin");

	m_pStateMachine->Update();
	const std::vector<AnimationKey>& basePose = m_pStateMachine->GetOutputPose();

	if (INPUT->GetButtonPressed(VK_RSHIFT)) {
		CacheAnimatioKey("Rifle Aiming Idle");
		std::vector<AnimationKey> blendedPose;
		m_pBlendMachine->Blend(basePose, m_mtxCachedPose, blendedPose);

		for (int i = 0; i < ownerBones.size(); ++i) {
			m_mtxCachedBoneTransforms[i] = blendedPose[i].CreateSRT();
		}
	}
	else {
		for (int i = 0; i < ownerBones.size(); ++i) {
			m_mtxCachedBoneTransforms[i] = basePose[i].CreateSRT();
		}
	}

}
