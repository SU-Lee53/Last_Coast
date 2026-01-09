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

void LayeredBlendMachine::Blend(const std::vector<Bone>& bones, const std::vector<AnimationKey>& basePose, const std::vector<AnimationKey>& blendPose, std::vector<Matrix>& outmtxLocalMatrics) const
{
	int nBones = bones.size();
	outmtxLocalMatrics.resize(nBones);

	std::vector<Matrix> mtxBasePoseComponentSapce;
	std::vector<Matrix> mtxBlendPoseComponentSapce;
	BuildComponentSpace(bones, basePose, mtxBasePoseComponentSapce);
	BuildComponentSpace(bones, blendPose, mtxBlendPoseComponentSapce);

	std::vector<Matrix> mtxFinalComponentSpace(nBones);
	for (int i = 0; i < nBones; ++i) {
		if (bLayerMask[i].bMask) {
			Vector3 v3BaseScale, v3BaseTranslate;
			Vector3 v3BlendScale, v3BlendTranslate;
			Quaternion v4BaseRotation, v4BlendRotation;

			mtxBasePoseComponentSapce[i].Decompose(v3BaseScale, v4BaseRotation, v3BaseTranslate);
			mtxBlendPoseComponentSapce[i].Decompose(v3BlendScale, v4BlendRotation, v3BlendTranslate);

			AnimationKey finalKey{};
			finalKey.v3Scale = Vector3::Lerp(v3BaseScale, v3BlendScale, bLayerMask[i].fWeight);
			finalKey.v3Translation = Vector3::Lerp(v3BaseTranslate, v3BlendTranslate, bLayerMask[i].fWeight);
			finalKey.v4RotationQuat = Quaternion::Slerp(v4BaseRotation, v4BlendRotation, bLayerMask[i].fWeight);

			mtxFinalComponentSpace[i] = finalKey.CreateSRT();
		}
		else {
			mtxFinalComponentSpace[i] = mtxBasePoseComponentSapce[i];
		}
	}

	for (int i = 0; i < nBones; ++i) {
		int nParentIndex = bones[i].nParentIndex;
		if (nParentIndex >= 0) {
			// CS * Inverse(ParentCS) = Local
			Matrix mtxInverseParent = mtxFinalComponentSpace[nParentIndex].Invert();
			outmtxLocalMatrics[i] = mtxFinalComponentSpace[i] * mtxInverseParent;
		}
		else {
			outmtxLocalMatrics[i] = mtxFinalComponentSpace[i];
		}
	}
	
}

void LayeredBlendMachine::BuildComponentSpace(const std::vector<Bone>& bones, const std::vector<AnimationKey>& pose, std::vector<Matrix>& outComponentSpace)
{
	int nBones = bones.size();
	outComponentSpace.resize(nBones);
	for (int i = 0; i < nBones; ++i) {
		Matrix mtxLocal = pose[i].CreateSRT();
		int nParentIndex = bones[i].nParentIndex;

		outComponentSpace[i] = nParentIndex >= 0 ? (mtxLocal * outComponentSpace[nParentIndex]) : mtxLocal;
	}
}

float LayeredBlendMachine::ComputeBlendWeight(int nRelativeDepth, int maxDepth)
{
	if (maxDepth <= 0) {
		return 1.0f;
	}

	float fValue = (float)nRelativeDepth / (float)maxDepth;
	return ::SmoothStep(fValue, 0.f, 1.f);
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
		Matrix mtxToRoot = nParentIndex >= 0 ? m_mtxFinalBoneTransforms[i] * mtxToRootTransforms[nParentIndex] : m_mtxFinalBoneTransforms[i];
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
	m_mtxFinalBoneTransforms.resize(MAX_BONE_TRANSFORMS);

	m_pBlendMachine = std::make_unique<LayeredBlendMachine>(pOwner, "Spine", 3);
}

void PlayerAnimationController::ComputeAnimation()
{
	const std::vector<Bone>& ownerBones = GetOwnerBones();
	const std::vector<AnimationKey>& basePose = m_pStateMachine->GetOutputPose();

	if (INPUT->GetButtonPressed(VK_RBUTTON)) {
		CacheAnimatioKey("Rifle Aiming Idle");
		m_pBlendMachine->Blend(ownerBones, basePose, m_mtxCachedPose, m_mtxFinalBoneTransforms);

		//for (int i = 0; i < ownerBones.size(); ++i) {
		//	m_mtxCachedBoneTransforms[i] = blendedPose[i].CreateSRT();
		//}
	}
	else {
		for (int i = 0; i < ownerBones.size(); ++i) {
			m_mtxFinalBoneTransforms[i] = basePose[i].CreateSRT();
		}
	}

}
