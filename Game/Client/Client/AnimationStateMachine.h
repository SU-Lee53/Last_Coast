#pragma once

class Animation;
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
	void Initialize(std::shared_ptr<GameObject> pOwner);
	void Update();

	void ComputeAnimation(std::vector<Matrix>& boneTransforms, double dTimeElapsed) const;

	std::shared_ptr<AnimationState> GetCurrentAnimationState() const { return m_pCurrentState; }

private:
	virtual void InitializeStateGraph() = 0;

protected:
	std::vector<std::shared_ptr<AnimationState>> m_pStates;
	std::shared_ptr<AnimationState> m_pCurrentState = nullptr;
	std::weak_ptr<GameObject> m_wpOwner;
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


