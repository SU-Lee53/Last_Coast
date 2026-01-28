#include "pch.h"
#include "AnimationMontage.h"

void AnimationMontage::Initialize(std::shared_ptr<IGameObject> pOwner)
{
	m_wpOwner = pOwner;

	const auto& bones = pOwner->GetComponent<Skeleton>()->GetBones();
	m_OutputPose.resize(bones.size());
	//std::transform(bones.begin(), bones.end(), std::back_inserter(m_OutputPose), [](const Bone& b) { return b.mtxTransform; });

	BuildMontage();

	int nIndex{};
	float fTime = 0.f;
	for (auto& section : m_MontageSections) {
		section.fStartTime = fTime;
		section.fEndTime = fTime + section.pAnimationToPlay->GetDuration();
		fTime = section.fEndTime;

		m_SectionIndexMap.insert({ section.strName, nIndex });
		nIndex++;
	}

	m_fTotalDuration = fTime;

}

void AnimationMontage::Update()
{
	if (m_bBlendingOut == false && m_nCurrentSection < 0) {
		return;
	}

	float fPrevSectionTime = m_fSectionPlayTime;
	int nPrevSectionIndex = m_nCurrentSection;

	float fCurrentSectionTime = 0.f;
	int nCurrentSectionIndex = m_nCurrentSection;

	float fDeltaTime = DT;
	m_fTotalPlaytime += fDeltaTime;
	m_fSectionPlayTime += fDeltaTime;

	// Blend In / Out 처리
	if (m_bBlendingOut) {
		m_fBlendOutElapsed += DT;
		m_fBlendWeight = 1.0f - (m_fBlendOutElapsed / m_fBlendOutTime);
		if (m_fBlendWeight <= 0.f) {
			m_bBlendingOut = false;
			m_bPlaying = false;
			m_nCurrentSection = -1;
			m_fBlendWeight = 0.f;
			return;
		}
	}
	else {
		// Section 처리
		SECTION_TRANSITION eResult = HandleSection();
		fCurrentSectionTime = m_fSectionPlayTime;
		nCurrentSectionIndex = m_nCurrentSection;

		// Notify 처리
		switch (eResult)
		{
		case SECTION_TRANSITION::NONE:
		{
			HandleNotifies(fPrevSectionTime, fCurrentSectionTime, m_nCurrentSection);
			break;
		}
		case SECTION_TRANSITION::LOOP:
		{
			const auto& currentMontage = m_MontageSections[m_nCurrentSection];
			float fSectionDuration = currentMontage.fEndTime - currentMontage.fStartTime;
			HandleNotifies(fPrevSectionTime, fSectionDuration, nCurrentSectionIndex);
			HandleNotifies(0.f, fCurrentSectionTime, nCurrentSectionIndex);
			break;
		}

		case SECTION_TRANSITION::JUMPED:
		{
			HandleNotifies(0.f, fCurrentSectionTime, nCurrentSectionIndex);	// 새로 바뀐 Section 만 처리함
			break;
		}
		case SECTION_TRANSITION::STOPPED:
		default:
		{
			break;
		}
		}

		// Blend In 처리
		if (m_fBlendInTime > 0.f && m_fTotalPlaytime < m_fBlendInTime) {
			m_fBlendWeight = m_fTotalPlaytime / m_fBlendInTime;
			if (m_fBlendWeight >= 1.f) {
				m_fBlendWeight = 1.f;
			}
		}
	}

	// Weight SmoothStep
	m_fBlendWeight = ::SmoothStep01(m_fBlendWeight);

	// 최종 Pose 계산
	const auto& currentSection = m_MontageSections[m_nCurrentSection];
	float fTimeToPlay = m_fSectionPlayTime + currentSection.fStartTime;
	const auto& bones = m_wpOwner.lock()->GetComponent<Skeleton>()->GetBones();
	for (const auto& bone : bones) {
		m_OutputPose[bone.nIndex] = currentSection.pAnimationToPlay->GetKeyFrameSRT(bone.strBoneName, fTimeToPlay, bone.mtxTransform);
	}
}

void AnimationMontage::PlayMontage(const std::string& strSectionName)
{
	auto it = m_SectionIndexMap.find(strSectionName);
	if (it == m_SectionIndexMap.end()) {
		__debugbreak();
		return;
	}

	m_bPlaying = true;
	m_nCurrentSection = it->second;
	m_fTotalPlaytime = 0.f;
	m_fSectionPlayTime = 0.f;
}

void AnimationMontage::StopMontage()
{
	if (m_bPlaying) {
		//m_bPlaying = false;
		m_bBlendingOut = true;
		m_fBlendOutElapsed = 0.f;
	}
}

void AnimationMontage::JumpToSection(const std::string& strSectionName)
{
	if (!m_bPlaying) {
		return;
	}

	auto it = m_SectionIndexMap.find(strSectionName);
	if (it == m_SectionIndexMap.end()) {
		__debugbreak();
		return;
	}

	m_nCurrentSection = it->second;
	m_fSectionPlayTime = 0.f;
}

SECTION_TRANSITION AnimationMontage::HandleSection()
{
	const auto& currentSection = m_MontageSections[m_nCurrentSection];
	float fSectionDuration = currentSection.fEndTime - currentSection.fStartTime;

	// 아직 끝나지 않았다면 그냥 하던대로 재생하게 리턴
	if (m_fSectionPlayTime < fSectionDuration) {
		return SECTION_TRANSITION::NONE;
	}

	SECTION_TRANSITION eResult = SECTION_TRANSITION::NONE;

	switch (currentSection.eEndRule) {
	case MONTAGE_SECTION_END_RULE::NEXT:
	{
		if (m_nCurrentSection + 1 < m_MontageSections.size()) {
			m_nCurrentSection++;
			m_fSectionPlayTime = 0.f;
			eResult = SECTION_TRANSITION::JUMPED;
		}
		else {
			StopMontage();
			eResult = SECTION_TRANSITION::STOPPED;
		}
		break;
	}
	case MONTAGE_SECTION_END_RULE::LOOP:
	{
		m_fSectionPlayTime = 0.f;
		eResult = SECTION_TRANSITION::LOOP;
		break;
	}
	case MONTAGE_SECTION_END_RULE::JUMP:
	{
		auto it = m_SectionIndexMap.find(currentSection.strJumpTarget);
		if (it != m_SectionIndexMap.end()) {
			m_nCurrentSection = it->second;
			m_fSectionPlayTime = 0.f;
			eResult = SECTION_TRANSITION::JUMPED;
		}
		else {
			StopMontage();
			eResult = SECTION_TRANSITION::STOPPED;
		}
		break;
	}
	case MONTAGE_SECTION_END_RULE::STOP:
	default:
	{
		if (!m_bBlendingOut) {
			StopMontage();
			eResult = SECTION_TRANSITION::STOPPED;
		}
		break;
	}
	}

	return eResult;
}

void AnimationMontage::HandleNotifies(float fPrevTime, float fCurrentTime, int nSectionIndex)
{
	// Notify 처리
	auto pOwner = m_wpOwner.lock();
	if (!pOwner) {
		return;
	}

	for (auto& notify : m_Notifies) {
		if (notify.nSectionIndex != nSectionIndex) {
			continue;
		}
		
		float fTime = notify.fTime;
		if (fTime >= fPrevTime && fTime < fCurrentTime) {
			if (notify.pCallback) {
				notify.pCallback(pOwner);
			}
		}
	}
}

void AnimationMontage::UpdateFallback()
{
	if (auto pOwner = m_wpOwner.lock()) {
		const auto& bones = m_wpOwner.lock()->GetComponent<Skeleton>()->GetBones();
		m_OutputPose.clear();	// Capacity 는 유지됨
		//std::transform(bones.begin(), bones.end(), std::back_inserter(m_OutputPose), [](const Bone& b) { return b.mtxTransform; });
	}
}

void PlayerAnimationMontage::BuildMontage()
{
	// 1. Fire
	MontageSection fireSection{};
	fireSection.strName = "Rifle Fire";
	fireSection.pAnimationToPlay = ANIMATION->Get("Firing Rifle");
	fireSection.eEndRule = MONTAGE_SECTION_END_RULE::NEXT;
	m_MontageSections.push_back(fireSection);

	// 2. Aim Idle
	MontageSection aimSection{};
	aimSection.strName = "Rifle Aiming Idle";
	aimSection.pAnimationToPlay = ANIMATION->Get("Rifle Aiming Idle");
	aimSection.eEndRule = MONTAGE_SECTION_END_RULE::LOOP;
	m_MontageSections.push_back(aimSection);


}
