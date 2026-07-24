#pragma once

enum class MONTAGE_SECTION_END_RULE {
	NEXT,	// 다음 Section 으로
	LOOP,	// 자기 자신 Loop
	JUMP,	// 특정 Section 으로
	STOP,	// Montage 중지
	FREEZE	// 마지막 프레임 고정
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
	std::vector<size_t> channelIndices;

	MONTAGE_SECTION_END_RULE eEndRule = MONTAGE_SECTION_END_RULE::NEXT;
	std::string strJumpTarget;
};

struct MontageNotify : AnimationNotify {
	int nSectionIndex = -1;
};

class AnimationMontage {
public:
	void Initialize(std::shared_ptr<IGameObject> pOwner);
	void Update();

	void PlayMontage(const std::string& strSectionName);
	void StopMontage();

	// 외부 개입 Section 전환용
	void JumpToSection(const std::string& strSectionName);

	// 현재 섹션을 길이 비율(0~1) 지점에서 일시정지 — 그 프레임 자세로 유지.
	// PlayMontage 직후 호출. 이후 notify는 재개 전까지 발화하지 않는다 (수류탄 와인드업 홀드용).
	void PauseAtRatio(float fRatio);
	void ResumeMontage() { m_fPauseTime = -1.f; }
	bool IsPausedHold() const { return m_fPauseTime >= 0.f; }

	const std::vector<AnimationKey>& GetOutputPose() const { return m_OutputPose; }
	float GetBlendWeight() const { return m_fBlendWeight; }
	bool IsFreezed() const { return m_bFreezed; }
	bool IsPlaying() const { return m_bPlaying; }

protected:
	virtual void BuildMontage() {}
	SECTION_TRANSITION HandleSection();
	void HandleNotifies(float fPrevTime, float fCurrentTime, int nSectionIndex);
	void UpdateFallback();

protected:
	std::weak_ptr<IGameObject>	m_wpOwner;

	float m_fTotalPlaytime = 0.f;	// PlayMontage() 시작부터 총 재생시간
	float m_fTotalDuration = 0.f;	// 전채 재생시간

	int m_nCurrentSection = -1;		// 현재 Section
	float m_fSectionPlayTime = 0.f;	// 현재 재생중인 Section 시간
	float m_fPauseTime = -1.f;		// 섹션 로컬 일시정지 시점 (<0 = 없음). 도달 시 시간 고정

	float m_fBlendInTime = 0.2f;
	float m_fBlendOutTime = 0.2f;
	float m_fBlendOutElapsed = 0.0f;
	float m_fBlendWeight = 0.f;
	bool  m_bBlendingOut = false;

	bool m_bPlaying = false;
	bool m_bAdditive = false;
	bool m_bFreezed = false;

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


class ZombieAnimationMontage : public AnimationMontage {
public:
	virtual void BuildMontage() override;
};
