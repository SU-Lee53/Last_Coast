#include "pch.h"
#include "AnimationHelper.h"
#include "GameObject.h"

//////////////////////////////////////////////////////////////////////////
// LayeredBlendMachine

LayeredBlendMachine::LayeredBlendMachine(std::shared_ptr<IGameObject> pGameObject, const std::string& strBranch, int nBlendDepth)
	: strBranchBoneName{ strBranch }
{
	const auto& ownerBones = pGameObject->GetComponent<Skeleton>()->GetBones();
	int nBranchIndex = pGameObject->GetComponent<Skeleton>()->FindBoneIndex(strBranch);
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

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pose -> Transform
void AnimationHepler::LocalPoseToComponent(const std::vector<Bone>& bones, const std::vector<AnimationKey>& localPose, std::vector<Matrix>& outComponentSpace)
{
	int nBones = bones.size();
	outComponentSpace.resize(nBones);
	for (int i = 0; i < nBones; ++i) {
		Matrix mtxLocal = localPose[i].CreateSRT();
		int nParentIndex = bones[i].nParentIndex;

		outComponentSpace[i] = nParentIndex < 0 ? mtxLocal : mtxLocal * outComponentSpace[nParentIndex];
	}
}

void AnimationHepler::ComponentPoseToLocal(const std::vector<Bone>& bones, const std::vector<AnimationKey>& componentPose, std::vector<Matrix>& outLocalSpace)
{
	int nBones = bones.size();
	outLocalSpace.resize(nBones);
	std::vector<Matrix> mtxInverse(nBones);
	for (int i = 0; i < nBones; ++i) {
		mtxInverse[i] = componentPose[i].CreateSRT().Invert();
	}

	for (int i = 0; i < nBones; ++i) {
		Matrix mtxComponentSpace = componentPose[i].CreateSRT();
		const int nParentIndex = bones[i].nParentIndex;
		outLocalSpace[i] = nParentIndex < 0 ? mtxComponentSpace : mtxComponentSpace * mtxInverse[nParentIndex];
	}
}

void AnimationHepler::ComponentPoseToLocalWithInverseHint(const std::vector<Bone>& bones, const std::vector<AnimationKey>& componentPose, const std::vector<Matrix>& inverseComponent, std::vector<Matrix>& outLocalSpace)
{
	int nBones = bones.size();
	outLocalSpace.resize(nBones);
	for (int i = 0; i < nBones; ++i) {
		Matrix mtxComponentSpace = componentPose[i].CreateSRT();
		const int nParentIndex = bones[i].nParentIndex;
		outLocalSpace[i] = nParentIndex < 0 ? mtxComponentSpace : mtxComponentSpace * inverseComponent[nParentIndex];
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Transform -> Transform

void AnimationHepler::LocalToComponent(const std::vector<Bone>& bones, const std::vector<Matrix>& localTransform, std::vector<Matrix>& outComponentSpace)
{
	int nBones = bones.size();
	outComponentSpace.resize(nBones);
	for (int i = 0; i < nBones; ++i) {
		Matrix mtxLocal = localTransform[i];
		int nParentIndex = bones[i].nParentIndex;

		outComponentSpace[i] = nParentIndex < 0 ? mtxLocal : mtxLocal * outComponentSpace[nParentIndex];
	}
}

void AnimationHepler::ComponentToLocal(const std::vector<Bone>& bones, const std::vector<Matrix>& componentTransform, std::vector<Matrix>& outLocalSpace)
{
	int nBones = bones.size();
	outLocalSpace.resize(nBones);
	std::vector<Matrix> mtxInverse(nBones);
	for (int i = 0; i < nBones; ++i) {
		mtxInverse[i] = componentTransform[i].Invert();
	}

	for (int i = 0; i < nBones; ++i) {
		Matrix mtxComponentSpace = componentTransform[i];
		const int nParentIndex = bones[i].nParentIndex;
		outLocalSpace[i] = nParentIndex < 0 ? mtxComponentSpace : mtxComponentSpace * mtxInverse[nParentIndex];
	}
}

void AnimationHepler::ComponentToLocalWithInverseHint(const std::vector<Bone>& bones, const std::vector<Matrix>& componentTransform, const std::vector<Matrix>& inverseComponent, std::vector<Matrix>& outLocalSpace)
{
	int nBones = bones.size();
	outLocalSpace.resize(nBones);
	for (int i = 0; i < nBones; ++i) {
		Matrix mtxComponentSpace = componentTransform[i];
		const int nParentIndex = bones[i].nParentIndex;
		outLocalSpace[i] = nParentIndex < 0 ? mtxComponentSpace : mtxComponentSpace * inverseComponent[nParentIndex];
	}
}

void AnimationHepler::TransformModifyBone(const std::vector<Bone>& bones, int boneIndexToModify, const std::vector<Matrix>& localTransform, std::vector<Matrix>& componentTransform, const Quaternion& addRotation, float alpha)
{
	AnimationKey animKey{};
	componentTransform[boneIndexToModify].Decompose(animKey.v3Scale, animKey.v4RotationQuat, animKey.v3Translation);

	Quaternion targetRotation = animKey.v4RotationQuat * addRotation;
	Quaternion finalRot = Quaternion::Slerp(animKey.v4RotationQuat, targetRotation, alpha);
	animKey.v4RotationQuat = finalRot;

	componentTransform[boneIndexToModify] = animKey.CreateSRT();

	UpdateSubtree(bones, boneIndexToModify, localTransform, componentTransform);
}

void AnimationHepler::UpdateSubtree(const std::vector<Bone>& bones, int boneIndex, const std::vector<Matrix>& localTransform, std::vector<Matrix>& componentTransform)
{
	std::vector<const Bone*> DFSStack;
	DFSStack.reserve(bones.size());
	for (int i = 0; i < bones[boneIndex].nChildren; ++i) {
		DFSStack.push_back(&bones[bones[boneIndex].nChilerenIndex[i]]);
	}

	while (DFSStack.size() != 0) {
		const Bone* pCurBone = DFSStack.back();
		DFSStack.pop_back();

		int nIndex = pCurBone->nIndex;
		int nParentIndex = pCurBone->nParentIndex;
		componentTransform[nIndex] = localTransform[nIndex] * componentTransform[nParentIndex];

		for (int i = 0; i < pCurBone->nChildren; ++i) {
			DFSStack.push_back(&bones[pCurBone->nChilerenIndex[i]]);
		}
	}
}

