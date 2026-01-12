#include "pch.h"
#include "AnimationController.h"

LayeredBlendMachine::LayeredBlendMachine(std::shared_ptr<GameObject> pGameObject, const std::string& strBranch, int nBlendDepth)
	: strBranchBoneName{ strBranch }
{
	const auto& ownerBones = pGameObject->GetBones();
	int nBranchIndex = pGameObject->FindBoneIndex(strBranch);
	int nBones = ownerBones.size();
	bLayerMask.assign(nBones, LayerMask{});

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

		int nRelativeDepth = pCurBone->nDepth - nBlendRootDepth;

		bLayerMask[pCurBone->nIndex].bMask = TRUE;
		bLayerMask[pCurBone->nIndex].fWeight = ComputeBlendWeight(nRelativeDepth, nBlendDepth);

		for (int i = 0; i < pCurBone->nChildren; ++i) {
			DFSStack.push_back(&ownerBones[pCurBone->nChilerenIndex[i]]);
		}
	}
}

void LayeredBlendMachine::Blend(const std::vector<Bone>& bones, const std::vector<AnimationKey>& basePose, const std::vector<AnimationKey>& blendPose, std::vector<Matrix>& outmtxLocalMatrics, float fBlendWeight) const
{
	int nBones = bones.size();
	outmtxLocalMatrics.resize(nBones);

	std::vector<Matrix> mtxBasePoseComponentSapce;
	std::vector<Matrix> mtxBlendPoseComponentSapce;
	AnimationHepler::LocalPoseToComponent(bones, basePose, mtxBasePoseComponentSapce);
	AnimationHepler::LocalPoseToComponent(bones, blendPose, mtxBlendPoseComponentSapce);

	std::vector<Matrix> mtxFinalCSTransform(nBones);
	std::vector<Matrix> mtxFinalCSTransformInverse(nBones);
	for (int i = 0; i < nBones; ++i) {
		if (bLayerMask[i].bMask == TRUE) {	// 상체
			const float fWeight = bLayerMask[i].fWeight * fBlendWeight;

			AnimationKey baseKey{};
			AnimationKey blendKey{};

			mtxBasePoseComponentSapce[i].Decompose(baseKey.v3Scale, baseKey.v4RotationQuat, baseKey.v3Translation);
			mtxBlendPoseComponentSapce[i].Decompose(blendKey.v3Scale, blendKey.v4RotationQuat, blendKey.v3Translation);

			mtxFinalCSTransform[i] = AnimationKey::Lerp(baseKey, blendKey, fWeight).CreateSRT();
			mtxFinalCSTransformInverse[i] = mtxFinalCSTransform[i].Invert();
		}
		else {
			mtxFinalCSTransform[i] = mtxBasePoseComponentSapce[i];
			mtxFinalCSTransformInverse[i] = mtxFinalCSTransform[i].Invert();
		}
	}

	AnimationHepler::ComponentToLocalWithInverseHint(bones, mtxFinalCSTransform, mtxFinalCSTransformInverse, outmtxLocalMatrics);
}

float LayeredBlendMachine::ComputeBlendWeight(int nRelativeDepth, int maxDepth)
{
	if (maxDepth <= 0) {
		return 1.0f;
	}

	float fValue = (float)nRelativeDepth / (float)maxDepth;
	return ::SmoothStep01(fValue);
}

void AnimationController::Update()
{
	m_fTotalTimeElapsed += DT;
	ProcessInput();

	if (m_pStateMachine) {
		m_pStateMachine->Update();
	}

	if (m_pAnimationMontage) {
		m_pAnimationMontage->Update();
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
		Matrix mtxToRoot = nParentIndex >= 0 ? m_mtxCachedLocalBoneTransforms[i] * mtxToRootTransforms[nParentIndex] : m_mtxCachedLocalBoneTransforms[i];
		mtxToRootTransforms[i] = mtxToRoot;
	}

	m_mtxFinalBoneTransforms.resize(nBones);
	for (int i = 0; i < nBones; ++i) {
		m_mtxFinalBoneTransforms[i] = ownerBones[i].mtxOffset * mtxToRootTransforms[i];
		m_mtxFinalBoneTransforms[i] = m_mtxFinalBoneTransforms[i].Transpose();
	}
}

const std::vector<Bone>& AnimationController::GetOwnerBones() const
{
	return m_wpOwner.lock()->GetBones();
}

void AnimationController::CacheAnimationKey(const std::string& strAnimationName)
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

	m_pAnimationMontage = std::make_unique<PlayerAnimationMontage>();
	m_pAnimationMontage->Initialize(pOwner);

	int nBones = pOwner->GetBones().size();
	m_mtxCachedLocalBoneTransforms.resize(nBones);
	m_mtxFinalBoneTransforms.resize(nBones);

	m_pBlendMachine = std::make_unique<LayeredBlendMachine>(pOwner, "Spine", 3);
}

void PlayerAnimationController::ComputeAnimation()
{
	const std::vector<Bone>& ownerBones = GetOwnerBones();
	const std::vector<AnimationKey>& basePose = m_pStateMachine->GetOutputPose();
	float fMontageBlendWeight = m_pAnimationMontage->GetBlendWeight();

	m_mtxCachedPose = m_pAnimationMontage->GetOutputPose();
	m_pBlendMachine->Blend(ownerBones, basePose, m_mtxCachedPose, m_mtxCachedLocalBoneTransforms, fMontageBlendWeight);

	//if (fMontageBlendWeight > 0.f) {
	//	m_mtxCachedPose = m_pAnimationMontage->GetOutputPose();
	//	m_pBlendMachine->Blend(ownerBones, basePose, m_mtxCachedPose, m_mtxCachedLocalBoneTransforms, fMontageBlendWeight);
	//}
	//else {
	//	m_mtxCachedLocalBoneTransforms.resize(ownerBones.size());
	//	for (int i = 0; i < basePose.size(); ++i) {
	//		m_mtxCachedLocalBoneTransforms[i] = basePose[i].CreateSRT();
	//	}
	//}
}

void PlayerAnimationController::ProcessInput()
{
	if (INPUT->GetButtonDown(VK_RBUTTON)) {
		m_pAnimationMontage->PlayMontage("Rifle Aiming Idle");
	}
	if (INPUT->GetButtonUp(VK_RBUTTON)) {
		m_pAnimationMontage->StopMontage();
	}

	if (INPUT->GetButtonPressed(VK_RBUTTON) && INPUT->GetButtonDown(VK_LBUTTON)) {
		m_pAnimationMontage->JumpToSection("Rifle Fire");
	}


}
