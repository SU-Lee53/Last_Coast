#pragma once
#include "AnimationStateMachine.h"
#include "AnimationMontage.h"

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


class AnimationController {
public:
	virtual void Initialize(std::shared_ptr<GameObject> pOwner) = 0;
	void Update();

	const std::vector<Matrix> GetFinalOutput() const;
	double GetElapsedTime() const { return m_fTotalTimeElapsed; }
	double GetCurrentAnimationDuration() const;

	const std::unique_ptr<AnimationMontage>& GetMontage() const { return m_pAnimationMontage; }

protected:
	virtual void ProcessInput() {}
	virtual void ComputeAnimation() {}
	void ComputeFinalMatrix();

protected:
	// Helper
	const std::vector<Bone>& GetOwnerBones() const;

	void CacheAnimationKey(const std::string& strAnimationName);

protected:
	std::unique_ptr<AnimationStateMachine>	m_pStateMachine;
	std::unique_ptr<AnimationMontage>		m_pAnimationMontage;
	std::weak_ptr<GameObject>				m_wpOwner;
	float m_fTotalTimeElapsed = 0;	// 시작부터 흐른시간

	// Output cache
	std::vector<AnimationKey> m_mtxCachedPose;
	std::vector<Matrix> m_mtxCachedLocalBoneTransforms;
	std::vector<Matrix> m_mtxFinalBoneTransforms;

};

class PlayerAnimationController : public AnimationController {
public:
	virtual void Initialize(std::shared_ptr<GameObject> pOwner) override;
	virtual void ComputeAnimation() override;

protected:
	virtual void ProcessInput() override;

private:
	std::unique_ptr<LayeredBlendMachine> m_pBlendMachine;


};
