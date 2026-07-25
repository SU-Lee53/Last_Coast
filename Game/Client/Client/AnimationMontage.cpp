#include "pch.h"
#include "AnimationMontage.h"
#include "Zombie.h"
#include "ThirdPersonPlayer.h"

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

		section.channelIndices.resize(bones.size(), INVALID_ID);
		for (const auto& bone : bones) {
			section.channelIndices[bone.nIndex] =
				section.pAnimationToPlay->GetChannelIndex(bone.strBoneName);
		}

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

	// 일시정지 지점 도달 — 시간 고정(그 자세 유지). 이후 notify는 재개 전까지 안 나감.
	if (m_fPauseTime >= 0.f && m_fSectionPlayTime >= m_fPauseTime) {
		m_fSectionPlayTime = m_fPauseTime;
		fPrevSectionTime = std::min(fPrevSectionTime, m_fPauseTime);
	}

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
	float fTimeToPlay = m_fSectionPlayTime;  // 섹션 내 로컬 시간으로 샘플링
	const auto& bones = m_wpOwner.lock()->GetComponent<Skeleton>()->GetBones();
	for (const auto& bone : bones) {
		m_OutputPose[bone.nIndex] = currentSection.pAnimationToPlay->GetKeyFrameSRT(
			currentSection.channelIndices[bone.nIndex], 
			fTimeToPlay, 
			bone.mtxTransform
		);
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
	m_bBlendingOut = false;
	m_fBlendOutElapsed = 0.f;
	m_nCurrentSection = it->second;
	m_fTotalPlaytime = 0.f;
	m_fSectionPlayTime = m_MontageSections[m_nCurrentSection].fStartOffset;
	m_fPauseTime = -1.f;	// 새 재생 = 이전 일시정지 예약 해제
	m_bFreezed = false;
}

void AnimationMontage::PauseAtRatio(float fRatio)
{
	if (m_nCurrentSection < 0) return;

	const auto& section = m_MontageSections[m_nCurrentSection];
	const float fDuration = section.fEndTime - section.fStartTime;
	m_fPauseTime = std::clamp(fRatio, 0.f, 1.f) * fDuration;
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
	case MONTAGE_SECTION_END_RULE::FREEZE:
	{
		// 마지막 프레임 고정 — 블렌드아웃 없이 weight 1.0 유지
		m_fSectionPlayTime = fSectionDuration;
		m_fBlendWeight = 1.0f;
		m_bFreezed = true;
		eResult = SECTION_TRANSITION::NONE;
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
	{
		MontageSection fireSection{};
		fireSection.strName = "Rifle Fire";
		fireSection.pAnimationToPlay = ANIMATION->Get("Firing Rifle");
		fireSection.eEndRule = MONTAGE_SECTION_END_RULE::NEXT;
		m_MontageSections.push_back(fireSection);
	}

	// 2. Rifle Aim Idle
	{
		MontageSection aimSection{};
		aimSection.strName = "Rifle Aiming Idle";
		aimSection.pAnimationToPlay = ANIMATION->Get("Rifle Aiming Idle");
		aimSection.eEndRule = MONTAGE_SECTION_END_RULE::LOOP;
		m_MontageSections.push_back(aimSection);
	}
	
	// 2. Rifle Reload
	{
		MontageSection reloadSection{};
		reloadSection.strName = "Rifle Reloading";
		reloadSection.pAnimationToPlay = ANIMATION->Get("Reloading");
		reloadSection.eEndRule = MONTAGE_SECTION_END_RULE::JUMP;
		reloadSection.strJumpTarget = "Rifle Aiming Idle";
		m_MontageSections.push_back(reloadSection);

		MontageNotify reloadEndNotify;
		reloadEndNotify.nSectionIndex = m_MontageSections.size() - 1;
		reloadEndNotify.fTime = ANIMATION->Get("Reloading")->GetDuration() - 0.1f;
		reloadEndNotify.pCallback = [](std::shared_ptr<IGameObject> pObj) {
			std::static_pointer_cast<IThirdPersonPlayer>(pObj)->OnReloadEnd();
		};
		m_Notifies.push_back(reloadEndNotify);
	}
	
	// 4. Pistol Fire
	{
		MontageSection fireSection{};
		fireSection.strName = "Pistol Fire";
		fireSection.pAnimationToPlay = ANIMATION->Get("Pistol Idle");
		fireSection.eEndRule = MONTAGE_SECTION_END_RULE::NEXT;
		m_MontageSections.push_back(fireSection);
	}

	// 5. Pistol Aim Idle
	{
		MontageSection aimSection{};
		aimSection.strName = "Pistol Aiming Idle";
		aimSection.pAnimationToPlay = ANIMATION->Get("Pistol Idle");
		aimSection.eEndRule = MONTAGE_SECTION_END_RULE::LOOP;
		m_MontageSections.push_back(aimSection);
	}

	// 6. Pistol Aim Idle
	{
		MontageSection meleeSection{};
		meleeSection.strName = "Melee Attack";
		meleeSection.pAnimationToPlay = ANIMATION->Get("Standing Melee Attack Horizontal");
		meleeSection.eEndRule = MONTAGE_SECTION_END_RULE::JUMP;
		meleeSection.strJumpTarget = "Rifle Fire";
		m_MontageSections.push_back(meleeSection);

		MontageNotify meleeEndNotify;
		meleeEndNotify.nSectionIndex = m_MontageSections.size() - 1;
		meleeEndNotify.fTime = ANIMATION->Get("Standing Melee Attack Horizontal")->GetDuration() - 0.1f;
		meleeEndNotify.pCallback = [](std::shared_ptr<IGameObject> pObj) {
			std::static_pointer_cast<IThirdPersonPlayer>(pObj)->OnMeleeEnd();
		};
		m_Notifies.push_back(meleeEndNotify);
	}


	// 7. Weapon Draw — 무기 슬롯 교체 모션 (권총/주무기 구분, 종료 notify로 교체 잠금 해제)
	{
		auto AddDrawSection = [this](const std::string& strSectionName, const std::string& strAnimName) {
			MontageSection drawSection{};
			drawSection.strName = strSectionName;
			drawSection.pAnimationToPlay = ANIMATION->Get(strAnimName);
			drawSection.eEndRule = MONTAGE_SECTION_END_RULE::STOP;
			m_MontageSections.push_back(drawSection);

			MontageNotify drawEndNotify;
			drawEndNotify.nSectionIndex = m_MontageSections.size() - 1;
			drawEndNotify.fTime = ANIMATION->Get(strAnimName)->GetDuration() - 0.1f;
			drawEndNotify.pCallback = [](std::shared_ptr<IGameObject> pObj) {
				std::static_pointer_cast<IThirdPersonPlayer>(pObj)->OnWeaponDrawEnd();
			};
			m_Notifies.push_back(drawEndNotify);
		};

		AddDrawSection("Pistol Draw", "Drawing Pistol");
		AddDrawSection("Rifle Draw", "Drawing Rifle");
		AddDrawSection("Bandage Draw", "Drawing Pistol");	// 붕대/수류탄 꺼내기 (뒷주머니 연출 — 권총 클립 재사용, 종료 notify → OnWeaponDrawEnd 잠금 해제)
	}

	// 2. Pistol Reload
	{
		MontageSection reloadSection{};
		reloadSection.strName = "Pistol Reloading";
		reloadSection.pAnimationToPlay = ANIMATION->Get("Pistol Reloading");
		reloadSection.eEndRule = MONTAGE_SECTION_END_RULE::JUMP;
		reloadSection.strJumpTarget = "Pistol Aiming Idle";
		m_MontageSections.push_back(reloadSection);

		MontageNotify reloadEndNotify;
		reloadEndNotify.nSectionIndex = m_MontageSections.size() - 1;
		reloadEndNotify.fTime = ANIMATION->Get("Pistol Reloading")->GetDuration() - 0.1f;
		reloadEndNotify.pCallback = [](std::shared_ptr<IGameObject> pObj) {
			std::static_pointer_cast<IThirdPersonPlayer>(pObj)->OnReloadEnd();
		};
		m_Notifies.push_back(reloadEndNotify);
	}

	// 8. Bandage Wrap — 붕대 감기 전용 클립 (자기/아군 별도 모션).
	// 종료는 클립 길이가 아니라 IThirdPersonPlayer::Update의 4초 타이머(BANDAGE_CAST_SECONDS)가
	// StopBandageAction → StopMontage 로 끊는다. 그래서 LOOP + notify 없음.
	{
		MontageSection bandageSection{};
		bandageSection.strName = "Bandage Wrap";
		bandageSection.pAnimationToPlay = ANIMATION->Get("bandage-wrap-character");
		bandageSection.eEndRule = MONTAGE_SECTION_END_RULE::LOOP;
		m_MontageSections.push_back(bandageSection);

		MontageSection allySection{};
		allySection.strName = "Bandage Ally Wrap";
		allySection.pAnimationToPlay = ANIMATION->Get("bandage-ally-wrap");
		allySection.eEndRule = MONTAGE_SECTION_END_RULE::LOOP;
		m_MontageSections.push_back(allySection);
	}

	// 9. Grenade Throw — 원본 "Throw" 클립 하나로 와인드업+던지기.
	// 와인드업: PlayMontage 후 PauseAtRatio(GRENADE_HOLD_RATIO)로 중간 일시정지(뒤로 든 자세 유지),
	// 좌클릭 뗌: ResumeMontage로 이어서 재생 → 릴리즈 notify(투사체 스폰) → 종료 notify(잠금 해제).
	{
		MontageSection throwSection{};
		throwSection.strName = "Grenade Throw";
		throwSection.pAnimationToPlay = ANIMATION->Get("Throw");
		throwSection.eEndRule = MONTAGE_SECTION_END_RULE::STOP;
		m_MontageSections.push_back(throwSection);

		// 손이 앞으로 나오는 시점에 투사체 스폰 — 홀드 지점(GRENADE_HOLD_RATIO) 이후여야 함
		MontageNotify releaseNotify;
		releaseNotify.nSectionIndex = m_MontageSections.size() - 1;
		releaseNotify.fTime = ANIMATION->Get("Throw")->GetDuration() * IThirdPersonPlayer::GRENADE_RELEASE_RATIO;
		releaseNotify.pCallback = [](std::shared_ptr<IGameObject> pObj) {
			std::static_pointer_cast<IThirdPersonPlayer>(pObj)->OnGrenadeRelease();
		};
		m_Notifies.push_back(releaseNotify);

		MontageNotify throwEndNotify;
		throwEndNotify.nSectionIndex = m_MontageSections.size() - 1;
		throwEndNotify.fTime = ANIMATION->Get("Throw")->GetDuration() - 0.1f;
		throwEndNotify.pCallback = [](std::shared_ptr<IGameObject> pObj) {
			std::static_pointer_cast<IThirdPersonPlayer>(pObj)->OnGrenadeThrowEnd();
		};
		m_Notifies.push_back(throwEndNotify);
	}

	// 10. Player Death — 사망 모션. FREEZE로 마지막 프레임(쓰러진 자세) 유지,
	// 부활 시 ApplyRespawn의 StopMontage가 해제해 상태머신으로 복귀.
	{
		MontageSection deathSection{};
		deathSection.strName = "Player Death";
		deathSection.pAnimationToPlay = ANIMATION->Get("Player Die");
		deathSection.eEndRule = MONTAGE_SECTION_END_RULE::FREEZE;
		deathSection.bFullBody = true;		// 하체까지 전신 재생 — 상체 마스크 블렌드로는 쓰러지는 모션 불가
		deathSection.fStartOffset = 0.45f;	// 클립 앞 ~0.5초는 서있는 대기 자세 — 스킵해 즉시 쓰러짐
		m_MontageSections.push_back(deathSection);
	}
}

void ZombieAnimationMontage::BuildMontage()
{
	MontageSection attackSection{};
	attackSection.strName = "Zombie Attack";
	attackSection.pAnimationToPlay = ANIMATION->Get("Zombie Attack");
	attackSection.eEndRule = MONTAGE_SECTION_END_RULE::STOP;
	m_MontageSections.push_back(attackSection);

	MontageSection deathSection{};
	deathSection.strName = "Zombie Death";
	deathSection.pAnimationToPlay = ANIMATION->Get("Zombie Death");
	deathSection.eEndRule = MONTAGE_SECTION_END_RULE::FREEZE;
	m_MontageSections.push_back(deathSection);

	MontageNotify attackSoundNotify{};
	attackSoundNotify.nSectionIndex = 0;
	attackSoundNotify.fTime = 0.85f;
	attackSoundNotify.pCallback = [](std::shared_ptr<IGameObject> pObj) {
		static const std::array<std::string, 12> strAttackSoundNames = {
			"zombie_attack_1",
			"zombie_attack_2",
			"zombie_attack_3",
			"zombie_attack_4",
			"zombie_attack_5",
			"zombie_attack_6",
			"zombie_attack_7",
			"zombie_attack_8",
			"zombie_attack_9",
			"zombie_attack_10",
			"zombie_attack_11",
			"zombie_attack_12"
		};

		const int nSoundIndex = RandomGenerator::GenerateRandomIntInRange(
			0,
			static_cast<int>(strAttackSoundNames.size()) - 1
		);
		SOUND->PlayAt(strAttackSoundNames[nSoundIndex], pObj->GetTransform()->GetPosition());
	};
	m_Notifies.push_back(attackSoundNotify);

	MontageNotify deathSoundNotify{};
	deathSoundNotify.nSectionIndex = 1;
	deathSoundNotify.fTime = 0.05f;
	deathSoundNotify.pCallback = [](std::shared_ptr<IGameObject> pObj) {
		static const std::array<std::string, 10> strDeathSoundNames = {
			"zombie_dead_1",
			"zombie_dead_2",
			"zombie_dead_3",
			"zombie_dead_4",
			"zombie_dead_5",
			"zombie_dead_6",
			"zombie_dead_7",
			"zombie_dead_8",
			"zombie_dead_9",
			"zombie_dead_10"
		};

		const int nSoundIndex = RandomGenerator::GenerateRandomIntInRange(
			0,
			static_cast<int>(strDeathSoundNames.size()) - 1
		);
		SOUND->PlayAt(strDeathSoundNames[nSoundIndex], pObj->GetTransform()->GetPosition());
	};
	m_Notifies.push_back(deathSoundNotify);

	MontageNotify hitNotify{};
	hitNotify.nSectionIndex = 0;
	hitNotify.fTime = 1.2f;
	hitNotify.pCallback = [](std::shared_ptr<IGameObject> pObj) {
		std::static_pointer_cast<Zombie>(pObj)->TriggerAttackHit();
	};
	m_Notifies.push_back(hitNotify);
}
