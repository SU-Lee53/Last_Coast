#pragma once
#include "Component.h"
#include "AnimationStateMachine.h"
#include "AnimationMontage.h"

class Skeleton;

class AnimationController : public IComponent {
public:
	AnimationController(std::shared_ptr<IGameObject> pOwner);

	virtual void Initialize() override;
	virtual void Update() override;

public:
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
	void CacheAnimationKey(const std::string& strAnimationName);
	const std::vector<Bone>& GetOwnerBones() const;

protected:
	std::unique_ptr<AnimationStateMachine>	m_pStateMachine;
	std::unique_ptr<AnimationMontage>		m_pAnimationMontage;
	float m_fTotalTimeElapsed = 0;	// 시작부터 흐른시간

	// Output cache
	std::vector<AnimationKey> m_mtxCachedPose;
	std::vector<Matrix> m_mtxCachedLocalBoneTransforms;
	std::vector<Matrix> m_mtxFinalBoneTransforms;

	// Bone Ref
	std::weak_ptr<Skeleton> m_wpOwnerSkeleton;
};

template <>
struct ComponentIndex<AnimationController> {
	constexpr static COMPONENT_TYPE componentType = COMPONENT_TYPE::ANIMATION_CONTROLLER;
	constexpr static std::underlying_type_t<COMPONENT_TYPE> index = std::to_underlying(COMPONENT_TYPE::ANIMATION_CONTROLLER);
};

class ThirdPersonCamera;

class PlayerAnimationController : public AnimationController {
public:
	PlayerAnimationController(std::shared_ptr<IGameObject> pOwner);

	// From IComponent
	void Initialize() override;
	virtual std::shared_ptr<IComponent> Copy(std::shared_ptr<IGameObject> pNewOwner)const override;

	// From AnimationController
	virtual void ComputeAnimation() override;

private:
	std::unique_ptr<LayeredBlendMachine> m_pBlendMachine;
	int m_nSpineIndex = 0;
	std::weak_ptr<ThirdPersonCamera> m_wpPlayerCamera;

};

template <>
struct ComponentIndex<PlayerAnimationController> {
	constexpr static COMPONENT_TYPE componentType = COMPONENT_TYPE::ANIMATION_CONTROLLER;
	constexpr static std::underlying_type_t<COMPONENT_TYPE> index = std::to_underlying(COMPONENT_TYPE::ANIMATION_CONTROLLER);
};
