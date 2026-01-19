#pragma once

enum class MONTAGE_SECTION_END_RULE {
	NEXT,	// 다음 Section 으로
	LOOP,	// 자기 자신 Loop
	JUMP,	// 특정 Section 으로
	STOP	// Montage 중지
};

enum class SECTION_TRANSITION {
	NONE,
	LOOP,
	JUMPED,
	STOPPED
};

struct MontageSection {
	std::string strName;
	std::shared_ptr<Animation> pAnimationToPlay;
	float fStartTime;
	float fEndTime;

	MONTAGE_SECTION_END_RULE eEndRule = MONTAGE_SECTION_END_RULE::NEXT;
	std::string strJumpTarget;
};

struct MontageNotify {
	int nSectionIndex;
	float fTime;
	std::function<void(std::shared_ptr<GameObject>)> pCallback;
};

class AnimationMontage {
public:
	void Initialize(std::shared_ptr<GameObject> pOwner);
	void Update();

	void PlayMontage(const std::string& strSectionName);
	void StopMontage();

	// 외부 개입 Section 전환용
	void JumpToSection(const std::string& strSectionName);

	const std::vector<AnimationKey>& GetOutputPose() const { return m_OutputPose; }
	float GetBlendWeight() const { return m_fBlendWeight; }

protected:
	virtual void BuildMontage() {}
	SECTION_TRANSITION HandleSection();
	void HandleNotifies(float fPrevTime, float fCurrentTime, int nSectionIndex);
	void UpdateFallback();

protected:
	std::weak_ptr<GameObject>	m_wpOwner;

	float m_fTotalPlaytime = 0.f;	// PlayMontage() 시작부터 총 재생시간
	float m_fTotalDuration = 0.f;	// 전채 재생시간

	int m_nCurrentSection = -1;		// 현재 Section
	float m_fSectionPlayTime = 0.f;	// 현재 재생중인 Section 시간

	float m_fBlendInTime = 0.2f;
	float m_fBlendOutTime = 0.2f;
	float m_fBlendOutElapsed = 0.0f;
	float m_fBlendWeight = 0.f;
	bool  m_bBlendingOut = false;

	bool m_bPlaying = false;
	bool m_bAdditive = false;

	std::unordered_map<std::string, UINT> m_SectionIndexMap;
	std::vector<MontageSection> m_MontageSections;
	std::vector<MontageNotify> m_Notifies;

	// Output cache
	std::vector<AnimationKey> m_OutputPose;
};

class PlayerAnimationMontage : public AnimationMontage {
public:
	virtual void BuildMontage() override;


};
