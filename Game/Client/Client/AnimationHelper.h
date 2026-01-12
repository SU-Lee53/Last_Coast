#pragma once


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
		std::vector<Matrix>& componentTransform,
		int boneIndex,
		const Quaternion& addRotation,
		float alpha);

};
