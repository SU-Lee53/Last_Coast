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
	AnimationStateMachine();

	void Initialize(std::shared_ptr<GameObject> pOwner);
	void Update();

	void ComputeAnimation();

	std::shared_ptr<AnimationState> GetCurrentAnimationState() const { return m_pCurrentState; }

	double GetElapsedTime() const { return m_dTotalTimeElapsed; }
	const std::vector<Matrix>& GetFinalMatrix() const { return m_mtxfinalBoneTransforms; }

private:
	virtual void InitializeStateGraph() = 0;

protected:
	std::vector<std::shared_ptr<AnimationState>> m_pStates;
	std::shared_ptr<AnimationState> m_pBeforeState = nullptr;
	std::shared_ptr<AnimationState> m_pCurrentState = nullptr;
	std::weak_ptr<GameObject> m_wpOwner;

	double m_dTotalTimeElapsed = 0;				// 시작부터 흐른시간
	double m_dCurrentAnimationTime = 0;			// 현재 애니메이션 시작부터 흐른 시간
	double m_dLastAnimationChangedTime = 0;		// 마지막 애니메이션 전환 시점
	double m_dCurrentTransitionTime = 0;		// 마지막 애니메이션 전환 시간

	std::vector<Matrix> m_mtxfinalBoneTransforms;	// Transposed

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


