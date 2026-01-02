#pragma once
#include "AnimationStateMachine.h"

class AnimationController {
public:
	virtual void Initialize(std::shared_ptr<GameObject> pOwner) = 0;
	void Update();

	const std::vector<Matrix> GetKeyframeSRT() const;
	double GetElapsedTime() const { return m_dTimeElapsed; }
	double GetCurrentAnimationDuration() const;

protected:
	std::unique_ptr<AnimationStateMachine> m_pStateMachine;
	std::weak_ptr<GameObject> m_wpOwner;
	double m_dTimeElapsed = 0;

	std::vector<Matrix> m_finalBoneTransforms;	// Transposed

};

class PlayerAnimationController : public AnimationController {
public:
	virtual void Initialize(std::shared_ptr<GameObject> pOwner) override;

};
