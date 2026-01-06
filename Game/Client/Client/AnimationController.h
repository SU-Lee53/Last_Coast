#pragma once
#include "AnimationStateMachine.h"

struct LayeredBlendMachine {
	std::vector<BOOL> bLayerMask;	// TRUE -> 상체 / FALSE -> 하체
	std::string strBranchBoneName;
	bool bInitialized = false;
	int nBones = 0;

	LayeredBlendMachine(std::shared_ptr<GameObject> pGameObject, const std::string& strBranch);
	void Blend(const std::vector<AnimationKey>& mtxBasePose, const std::vector<AnimationKey>& mtxBlendPose, std::vector<AnimationKey>& outOutputPose) const;

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
	std::vector<Matrix> m_mtxCachedBoneTransforms;
	std::vector<Matrix> m_mtxFinalBoneTransforms;	// Transposed

};

class PlayerAnimationController : public AnimationController {
public:
	virtual void Initialize(std::shared_ptr<GameObject> pOwner) override;
	virtual void ComputeAnimation() override;

private:
	std::unique_ptr<LayeredBlendMachine> m_pBlendMachine;


};
