#pragma once
#include "Animation.h"

struct AnimationState;

struct TransitionEdges {
	double dTransitionTime;
	std::weak_ptr<AnimationState> pConnectedState;
};

struct AnimationState {
	std::string strName;
	std::shared_ptr<Animation> pAnimationToPlay;
	UINT eAnimationPlayType;

	std::function<bool(std::shared_ptr<GameObject>)> fnStateTransitionCallback;
	std::vector<TransitionEdges> pConnectedEdges;

	void Connect(std::shared_ptr<AnimationState> pState, double dTransitionTime) {
		assert(*this != *pState);
		pConnectedEdges.push_back({ dTransitionTime, pState });
	}

	bool operator==(const AnimationState& other) const {
		return strName == other.strName;
	}
};

class AnimationStateMachine {
public:
	AnimationStateMachine();

	void Initialize(std::shared_ptr<GameObject> pOwner, float fInitialTime);
	void Update();

	std::shared_ptr<AnimationState> GetCurrentAnimationState() const { return m_pCurrentState; }

	const std::vector<AnimationKey>& GetOutputPose() const { return m_mtxOutputPose; }

private:
	virtual void InitializeStateGraph() = 0;

protected:
	std::vector<std::shared_ptr<AnimationState>> m_pStates;
	std::shared_ptr<AnimationState> m_pBeforeState = nullptr;
	std::shared_ptr<AnimationState> m_pCurrentState = nullptr;
	std::weak_ptr<GameObject> m_wpOwner;

	float m_fTotalAnimationTime = 0;			// 초기화부터 총 흐른시간
	float m_fLastAnimationChangedTime = 0;		// 마지막 애니메이션 전환 시점
	float m_fCurrentTransitionTime = 0;			// 마지막 애니메이션 전환 시간

	std::vector<AnimationKey> m_mtxOutputPose;

};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PlayerAnimationStateMachine

class PlayerAnimationStateMachine : public AnimationStateMachine {
public:
	virtual void InitializeStateGraph() override;

	static bool IdleCallback(std::shared_ptr<GameObject> pObj);
	static bool WalkCallback(std::shared_ptr<GameObject> pObj);
	static bool RunCallback(std::shared_ptr<GameObject> pObj);


};


