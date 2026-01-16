#include "pch.h"
#include "AnimationHelper.h"

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

