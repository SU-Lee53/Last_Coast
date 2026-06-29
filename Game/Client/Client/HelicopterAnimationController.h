#pragma once
#include "AnimationController.h"

// ──────────────────────────────────────────────────────────────────────────
// HelicopterAnimationStateMachine
//  Single state ("Fly") that loops the rotor animation. No transitions.
// ──────────────────────────────────────────────────────────────────────────
class HelicopterAnimationStateMachine : public AnimationStateMachine {
public:
	virtual void InitializeStateGraph() override;
};

// ──────────────────────────────────────────────────────────────────────────
// HelicopterAnimationController
//  - No camera, no montage
//  - Single looping clip only (rotor spin)
//  Needed so the skeletal helicopter mesh produces bone matrices; without an
//  AnimationController the GBufferPass skips bone data and the skinned mesh crashes.
// ──────────────────────────────────────────────────────────────────────────
class HelicopterAnimationController : public AnimationController {
public:
	HelicopterAnimationController(std::shared_ptr<IGameObject> pOwner);

	void Initialize() override;
	std::shared_ptr<IComponent> Copy(std::shared_ptr<IGameObject> pNewOwner) const override;

protected:
	virtual void ComputeAnimation() override;
};

template <>
struct ComponentIndex<HelicopterAnimationController> {
	constexpr static COMPONENT_TYPE componentType = COMPONENT_TYPE::ANIMATION_CONTROLLER;
	constexpr static std::underlying_type_t<COMPONENT_TYPE> index = std::to_underlying(COMPONENT_TYPE::ANIMATION_CONTROLLER);
};
