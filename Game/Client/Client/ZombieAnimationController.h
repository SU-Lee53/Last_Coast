#pragma once
#include "AnimationController.h"

// ──────────────────────────────────────────────────────────────────────────
// ZombieAnimationStateMachine
//  Idle ↔ Walk / Run (0.2s transition)
//  Transition condition: Zombie::GetMoveSpeedSqXZ() + IsFast()
//  Run = 빠른 좀비(30%) 전용 "Zombie Speed Running" 클립
// ──────────────────────────────────────────────────────────────────────────
class ZombieAnimationStateMachine : public AnimationStateMachine {
public:
	virtual void InitializeStateGraph() override;

	static bool IdleCallback(std::shared_ptr<IGameObject> pObj);
	static bool WalkCallback(std::shared_ptr<IGameObject> pObj);
	static bool RunCallback(std::shared_ptr<IGameObject> pObj);
};

// ──────────────────────────────────────────────────────────────────────────
// ZombieAnimationController
//  - No camera, no montage
//  - StateMachine only (Idle / Walk)
// ──────────────────────────────────────────────────────────────────────────
class ZombieAnimationController : public AnimationController {
public:
	ZombieAnimationController(std::shared_ptr<IGameObject> pOwner);

	void Initialize() override;
	std::shared_ptr<IComponent> Copy(std::shared_ptr<IGameObject> pNewOwner) const override;

protected:
	virtual void ComputeAnimation() override;
};

template <>
struct ComponentIndex<ZombieAnimationController> {
	constexpr static COMPONENT_TYPE componentType = COMPONENT_TYPE::ANIMATION_CONTROLLER;
	constexpr static std::underlying_type_t<COMPONENT_TYPE> index = std::to_underlying(COMPONENT_TYPE::ANIMATION_CONTROLLER);
};
