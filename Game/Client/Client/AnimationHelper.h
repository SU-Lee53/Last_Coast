#pragma once

class GameObject;

struct LayeredBlendMachine {
	struct LayerMask {
		BOOL bMask = FALSE;
		float fWeight = 0.f;
	};

	std::vector<LayerMask> bLayerMask;	// TRUE -> 상체 / FALSE -> 하체
	std::string strBranchBoneName;
	bool bInitialized = false;

	LayeredBlendMachine(
		std::shared_ptr<GameObject> pGameObject,
		const std::string& strBranch,
		int nBlendDepth);

	void Blend(
		const std::vector<Bone>& bones,
		const std::vector<AnimationKey>& BasePose,
		const std::vector<AnimationKey>& BlendPose,
		std::vector<Matrix>& outmtxLocalMatrics,
		float fBlendWeight = 1.f) const;

private:
	static float ComputeBlendWeight(int nRelativeDepth, int maxDepth);
};


struct AnimationHepler {
	/////////////////////////////////////////////////////////////
	// Pose -> Transform
	static void LocalPoseToComponent(
		const std::vector<Bone>& bones,
		const std::vector<AnimationKey>& localPose,
		std::vector<Matrix>& outComponentSpace);

	static void ComponentPoseToLocal(
		const std::vector<Bone>& bones,
		const std::vector<AnimationKey>& componentPose,
		std::vector<Matrix>& outLocalSpace);

	static void ComponentPoseToLocalWithInverseHint(
		const std::vector<Bone>& bones,
		const std::vector<AnimationKey>& componentPose,
		const std::vector<Matrix>& inverseComponent,
		std::vector<Matrix>& outLocalSpace);


	/////////////////////////////////////////////////////////////
	// Transform -> Transform
	static void LocalToComponent(
		const std::vector<Bone>& bones,
		const std::vector<Matrix>& localTransform,
		std::vector<Matrix>& outComponentSpace);

	static void ComponentToLocal(
		const std::vector<Bone>& bones,
		const std::vector<Matrix>& componentTransform,
		std::vector<Matrix>& outLocalSpace);

	static void ComponentToLocalWithInverseHint(
		const std::vector<Bone>& bones,
		const std::vector<Matrix>& componentTransform,
		const std::vector<Matrix>& inverseComponent,
		std::vector<Matrix>& outLocalSpace);


	/////////////////////////////////////////////////////////////
	// Bone Modifier
	static void TransformModifyBone(
		const std::vector<Bone>& bones,
		int boneIndexToModify,
		const std::vector<Matrix>& localTransform,
		std::vector<Matrix>& componentTransform,
		const Quaternion& addRotation,
		float alpha);

private:
	static void UpdateSubtree(
		const std::vector<Bone>& bones,
		int boneIndex,
		const std::vector<Matrix>& localTransform,
		std::vector<Matrix>& componentTransform);
};
