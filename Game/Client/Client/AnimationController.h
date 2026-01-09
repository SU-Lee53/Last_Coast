#pragma once
#include "AnimationStateMachine.h"

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
		std::vector<Matrix>& outmtxLocalMatrics) const;

	// 아래 헬퍼들은 나중에 어딘가로 옮기는게 좋아보임
	static void BuildComponentSpace(
		const std::vector<Bone>& bones,
		const std::vector<AnimationKey>& pose,
		std::vector<Matrix>& outComponentSpace);

	static float ComputeBlendWeight(int nRelativeDepth, int maxDepth);
};


class AnimationController {
public:
	virtual void Initialize(std::shared_ptr<GameObject> pOwner) = 0;
	void Update();

	const std::vector<Matrix> GetFinalOutput() const;
	double GetElapsedTime() const { return m_fTotalTimeElapsed; }
	double GetCurrentAnimationDuration() const;

protected:
	void ComputeFinalMatrix();
	virtual void ComputeAnimation() {};

	void LayerdBlendPerBone(const std::vector<AnimationKey>& mtxPose1, const std::vector<AnimationKey>& mtxPose2,
		const std::string& strBranchBoneName, float fBlendWeight = 1.f, int nBlendDepth = 0);

protected:
	// Helper
	const std::vector<Bone>& GetOwnerBones() const;

	void CacheAnimatioKey(const std::string& strAnimationName);

protected:
	std::unique_ptr<AnimationStateMachine> m_pStateMachine;
	std::weak_ptr<GameObject> m_wpOwner;
	float m_fTotalTimeElapsed = 0;	// 시작부터 흐른시간

	std::vector<AnimationKey> m_mtxCachedPose;
	std::vector<Matrix> m_mtxFinalBoneTransforms;

};

class PlayerAnimationController : public AnimationController {
public:
	virtual void Initialize(std::shared_ptr<GameObject> pOwner) override;
	virtual void ComputeAnimation() override;

private:
	std::unique_ptr<LayeredBlendMachine> m_pBlendMachine;


};
