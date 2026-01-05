#pragma once
#include "AnimationStateMachine.h"

class AnimationController {
public:
	virtual void Initialize(std::shared_ptr<GameObject> pOwner) = 0;
	void Update();

	const std::vector<Matrix> GetFinalOutput() const;
	double GetElapsedTime() const { return m_pStateMachine->GetElapsedTime(); }
	double GetCurrentAnimationDuration() const;

protected:
	std::unique_ptr<AnimationStateMachine> m_pStateMachine;
	std::weak_ptr<GameObject> m_wpOwner;

	std::vector<Matrix> m_finalBoneTransforms;	// Transposed

};

class PlayerAnimationController : public AnimationController {
public:
	virtual void Initialize(std::shared_ptr<GameObject> pOwner) override;

};
