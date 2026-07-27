#include "pch.h"
#include "GameEvent.h"
#include "EventSequence.h"
#include "Skybox.h"
#include "BloodEffect.h"
#include "FireEffect.h"
#include "CrashFireEffect.h"
#include "MuzzleFlashEffect.h"
#include "ExplosionEffect.h"
#include "GrenadeExplosionEffect.h"
#include "DecoyExplosionEffect.h"
#include "ToneMappingVolume.h"
#include "PostProcessingVolume.h"
#include "NodeObject.h"
#include "CrashDebris.h"
#include "HelicopterObject.h"
#include "SpatialTraits.h"
#include "Skeleton.h"

void ILoopEvent::Update(Scene* pScene)
{
	if (!m_bIsTriggered) {
		m_bIsTriggered = true;
		OnEnterEvent(pScene);
	}

	OnUpdateEvent(pScene);
}

void IIntervalEvent::Update(Scene* pScene)
{
	m_fTimeElapsed += DT;
	if (m_fTimeElapsed >= m_fInterval) {
		m_fTimeElapsed = 0.f;
		OnUpdateEvent(pScene);
	}
}

void ITriggerEvent::Update(Scene* pScene)
{
	if (m_fnEndTrigger() && m_bIsTriggered == true) {
		m_bIsTriggered = false;
		OnLeaveEvent(pScene);
		return;
	}

	if (m_fnBeginTrigger() && m_bIsTriggered == false) {
		m_bIsTriggered = true;
		OnEnterEvent(pScene);
	}

	if (m_bIsTriggered) {
		OnUpdateEvent(pScene);
	}

}

void TimeForwardEvent::OnUpdateEvent(Scene* pScene)
{
	auto& pSkybox = pScene->GetSkybox();
	
	const_cast<std::shared_ptr<Skybox>&>(pSkybox)->SetDayNightBlend(m_fTime);
	m_fTime = fmodf((m_fTime + (0.1f * DT)), 0.5f) + 0.5f;
}

void BleedEvent::Initialize(Scene* pScene)
{
	m_fInterval = 1.0f;
}

void BleedEvent::OnUpdateEvent(Scene* pScene)
{
	auto& pPlayer = pScene->GetPlayer();
	ParticleEffectSpawnDesc particleDesc;
	{
		particleDesc.v3Position = pPlayer->GetTransform()->GetPosition();
		particleDesc.v3Direction = pPlayer->GetTransform()->GetLook();
		particleDesc.v3Normal = pPlayer->GetTransform()->GetUp();
		particleDesc.mtxWorld = Matrix::CreateWorld(particleDesc.v3Position, particleDesc.v3Direction, particleDesc.v3Normal);
	};

	PARTICLE->Spawn<BloodEffect>(particleDesc);
}

void ExplosionEvent::OnEnterEvent(Scene* pScene)
{
	// 헬기 추락과 동일한 폭발 연출 — 사운드는 ExplosionEffect::Play가 3D로 재생
	ParticleEffectSpawnDesc desc;
	desc.v3Position  = m_v3Pos;
	desc.v3Direction = Vector3::Up;
	desc.v3Normal    = Vector3::Up;
	desc.mtxWorld    = Matrix::CreateWorld(desc.v3Position, desc.v3Direction, desc.v3Normal);
	PARTICLE->Spawn<ExplosionEffect>(desc);

	m_bFinished = true; // 1회 재생 후 즉시 종료
}

void GrenadeExplosionEvent::OnEnterEvent(Scene* pScene)
{
	// 수류탄 전용 소형 폭발 — 사운드는 GrenadeExplosionEffect::Play가 3D로 재생
	ParticleEffectSpawnDesc desc;
	desc.v3Position  = m_v3Pos;
	desc.v3Direction = Vector3::Up;
	desc.v3Normal    = Vector3::Up;
	desc.mtxWorld    = Matrix::CreateWorld(desc.v3Position, desc.v3Direction, desc.v3Normal);
	PARTICLE->Spawn<GrenadeExplosionEffect>(desc);

	m_bFinished = true; // 1회 재생 후 즉시 종료
}

void DecoyExplosionEvent::OnEnterEvent(Scene* pScene)
{
	// 디코이 전용 폭죽 연출 — 사운드는 DecoyExplosionEffect::Play가 3D로 재생
	ParticleEffectSpawnDesc desc;
	desc.v3Position  = m_v3Pos;
	desc.v3Direction = Vector3::Up;
	desc.v3Normal    = Vector3::Up;
	desc.mtxWorld    = Matrix::CreateWorld(desc.v3Position, desc.v3Direction, desc.v3Normal);
	PARTICLE->Spawn<DecoyExplosionEffect>(desc);

	m_bFinished = true; // 1회 재생 후 즉시 종료
}


// ── 환경 프리셋 틀 ────────────────────────────────────────────────────────────

// presetId → 프리셋 값 테이블. 새 룩은 여기에 항목만 추가하면 끝(이벤트 클래스 불필요).
const EnvironmentPreset& GetEnvironmentPreset(int presetId)
{
	// EP_DEFAULT — 모든 값 디폴트. 다른 프리셋에서 복구할 때 목표로 쓰임.
	static const EnvironmentPreset DEFAULT = [] {
		EnvironmentPreset p;            // 멤버 디폴트가 곧 기본 룩
		p.nEnableAutoExposure = 1;
		p.bAffectToneMapperMode = true;
		p.nToneMapperMode = static_cast<int>(TONE_MAPPING_MODE::ACES);
		return p;
	}();

	// EP_NIGHT — 챕터 1. 새벽 2시의 일반적인 좀비 액션 룩.
	static const EnvironmentPreset NIGHT = [] {
		EnvironmentPreset p;
		p.bAffectTime        = true;
		p.fTimeOfDayHour     = 2.0f;
		p.bAffectAmbient     = true;
		p.v4GlobalAmbient    = Vector4{ 0.070f, 0.080f, 0.12f, 1.0f };
		p.fExposure          = 1.13f;
		p.fPostSaturation    = 0.94f;
		p.fOutputScale       = 1.00f;
		p.fGradingStrength   = 0.66f;
		p.fTemperature       = -0.16f;
		p.fGradingContrast   = 1.12f;
		p.fGradingSaturation = 0.96f;
		p.v3ShadowTint       = Vector3{ 0.80f, 0.89f, 1.12f };
		p.fShadowWeight      = 0.28f;
		p.v3MidtoneTint      = Vector3{ 0.93f, 0.97f, 1.07f };
		p.fMidtoneWeight     = 0.16f;
		p.v3HighlightTint    = Vector3{ 0.91f, 0.97f, 1.12f };
		p.fHighlightWeight   = 0.16f;
		p.v3ColorFilter      = Vector3{ 0.88f, 0.95f, 1.11f };
		p.fColorFilterStrength = 0.16f;
		p.nEnableAutoExposure = 0;
		p.bAffectToneMapperMode = true;
		p.nToneMapperMode    = static_cast<int>(TONE_MAPPING_MODE::ACES);
		p.fBloomThreshold    = 1.12f;
		p.fBloomIntensity    = 0.46f;
		p.fBloomRadius       = 0.90f;
		p.fGrainStrength     = 0.016f;
		p.fGrainScale        = 1.10f;
		p.fVignetteStrength  = 0.28f;
		p.fVignetteRadius    = 0.80f;
		p.fVignetteSoftness  = 0.50f;
		p.fChromaticAberration = 3.20f;
		p.fHalationStrength    = 0.34f;
		p.fLightShaftIntensity = 0.18f;
		p.fLightShaftDensity   = 0.65f;
		p.fLightShaftWeight    = 0.04f;
		p.fLightShaftExposure  = 0.70f;
		p.v4FogColor              = Vector4{ 0.075f, 0.095f, 0.14f, 1.0f };
		p.fFogStartDistance       = 1800.f;
		p.fFogCutOffDistance      = 12000.f;
		p.fFogDistanceDensity     = 0.0035f;
		p.fFogDistancePower       = 1.40f;
		p.fFogHeightDensity       = 0.004f;
		p.fFogHeightFalloff       = 0.080f;
		p.fFogBaseHeightOffset    = -140.f;
		p.fFogHeightStartDistance = 1000.f;
		p.fFogMaxOpacity          = 0.075f;
		p.fFogDetailStrength      = 0.026f;
		p.fFogDetailNoiseScale    = 0.00035f;
		p.fFogDetailNoiseSpeed    = 0.012f;
		p.fFogDetailHeightRange   = 420.f;
		p.fFogDetailDistanceStart = 2200.f;
		p.fFogDetailDirectionalScattering = 0.04f;
		p.fMoonIntensity          = 3.8f;
		p.fSkyAmbientIntensity    = 0.32f;
		return p;
	}();

	// EP_DAWN — 챕터 2. 시야가 강하게 제한되는 음산한 사고 차량 구역.
	static const EnvironmentPreset DAWN = [] {
		EnvironmentPreset p;
		p.bAffectTime        = true;
		p.fTimeOfDayHour     = 3.5f;
		p.bAffectAmbient     = true;
		p.v4GlobalAmbient    = Vector4{ 0.085f, 0.090f, 0.085f, 1.0f };
		p.fExposure          = 0.90f;
		p.fPostSaturation    = 0.78f;
		p.fOutputScale       = 0.94f;
		p.fGamma             = 2.25f;
		p.fGradingStrength   = 0.84f;
		p.fTemperature       = -0.07f;
		p.fTint              = 0.01f;
		p.fGradingContrast   = 0.97f;
		p.fGradingSaturation = 0.78f;
		p.fGradingDensity    = 0.025f;
		p.v3ShadowTint       = Vector3{ 0.84f, 0.89f, 0.96f };
		p.fShadowWeight      = 0.30f;
		p.v3MidtoneTint      = Vector3{ 0.93f, 0.95f, 0.92f };
		p.fMidtoneWeight     = 0.24f;
		p.v3HighlightTint    = Vector3{ 1.03f, 1.00f, 0.90f };
		p.fHighlightWeight   = 0.20f;
		p.v3ColorFilter      = Vector3{ 0.92f, 0.93f, 0.87f };
		p.fColorFilterStrength = 0.20f;
		p.fBlackLift         = 0.018f;
		p.nEnableAutoExposure = 0;
		p.bAffectToneMapperMode = true;
		p.nToneMapperMode    = static_cast<int>(TONE_MAPPING_MODE::ACES);
		p.fBloomThreshold    = 0.92f;
		p.fBloomIntensity    = 0.48f;
		p.fBloomRadius       = 1.30f;
		p.fGrainStrength     = 0.055f;
		p.fGrainScale        = 2.20f;
		p.fVignetteStrength  = 0.46f;
		p.fVignetteRadius    = 0.68f;
		p.fVignetteSoftness  = 0.48f;
		p.fChromaticAberration = 6.00f;
		p.fHalationStrength    = 0.16f;
		p.fLightShaftIntensity = 0.08f;
		p.fLightShaftDensity   = 0.45f;
		p.fLightShaftWeight    = 0.025f;
		p.fLightShaftExposure  = 0.50f;
		p.v4FogColor              = Vector4{ 0.38f, 0.43f, 0.41f, 1.0f };
		p.fFogStartDistance       = 650.f;
		p.fFogCutOffDistance      = 3400.f;
		p.fFogDistanceDensity     = 0.029f;
		p.fFogDistancePower       = 0.85f;
		p.fFogHeightDensity       = 0.0018f;
		p.fFogHeightFalloff       = 0.010f;
		p.fFogBaseHeightOffset    = 10.f;
		p.fFogHeightStartDistance = 150.f;
		p.fFogMaxOpacity          = 0.82f;
		p.fFogDetailStrength      = 0.34f;
		p.fFogDetailNoiseScale    = 0.00050f;
		p.fFogDetailNoiseSpeed    = 0.025f;
		p.fFogDetailHeightRange   = 520.f;
		p.fFogDetailDistanceStart = 350.f;
		p.fFogDetailDirectionalScattering = 0.08f;
		p.fMoonIntensity          = 2.6f;
		p.fSkyAmbientIntensity    = 0.36f;
		return p;
	}();

	// EP_SUNSET — 챕터 3. 해가 뜬 뒤 구조 헬기가 보이는 따뜻한 탈출 구역.
	static const EnvironmentPreset SUNSET = [] {
		EnvironmentPreset p;
		p.bAffectTime        = true;
		p.fTimeOfDayHour     = 6.5f;
		p.bWatchSun          = false;
		p.bAffectAmbient     = true;
		p.v4GlobalAmbient    = Vector4{ 0.20f, 0.16f, 0.115f, 1.0f };
		p.fExposure          = 1.08f;
		p.fPostSaturation    = 1.08f;
		p.fOutputScale       = 1.02f;
		p.fGamma             = 2.20f;
		p.fGradingStrength   = 0.82f;
		p.fTemperature       = 0.20f;
		p.fTint              = 0.025f;
		p.fGradingContrast   = 1.05f;
		p.fGradingSaturation = 1.10f;
		p.fGradingDensity    = -0.025f;
		p.v3ShadowTint       = Vector3{ 0.94f, 0.92f, 0.86f };
		p.fShadowWeight      = 0.16f;
		p.v3MidtoneTint      = Vector3{ 1.06f, 1.00f, 0.88f };
		p.fMidtoneWeight     = 0.28f;
		p.v3HighlightTint    = Vector3{ 1.12f, 1.02f, 0.82f };
		p.fHighlightWeight   = 0.42f;
		p.v3ColorFilter      = Vector3{ 1.08f, 1.00f, 0.86f };
		p.fColorFilterStrength = 0.24f;
		p.fBlackLift         = 0.0f;
		p.nEnableAutoExposure = 0;
		p.bAffectToneMapperMode = true;
		p.nToneMapperMode    = static_cast<int>(TONE_MAPPING_MODE::ACES);
		p.fBloomThreshold    = 0.95f;
		p.fBloomIntensity    = 0.55f;
		p.fBloomRadius       = 1.30f;
		p.fGrainStrength     = 0.006f;
		p.fGrainScale        = 0.90f;
		p.fVignetteStrength  = 0.10f;
		p.fVignetteRadius    = 0.98f;
		p.fVignetteSoftness  = 0.62f;
		p.fChromaticAberration = 2.50f;
		p.fHalationStrength    = 0.62f;
		p.fLightShaftIntensity = 0.42f;
		p.fLightShaftDensity   = 0.80f;
		p.fLightShaftWeight    = 0.05f;
		p.fLightShaftExposure  = 0.80f;
		p.v4FogColor              = Vector4{ 0.58f, 0.48f, 0.34f, 1.0f };
		p.fFogStartDistance       = 2800.f;
		p.fFogCutOffDistance      = 15000.f;
		p.fFogDistanceDensity     = 0.0025f;
		p.fFogDistancePower       = 1.65f;
		p.fFogHeightDensity       = 0.003f;
		p.fFogHeightFalloff       = 0.080f;
		p.fFogBaseHeightOffset    = -160.f;
		p.fFogHeightStartDistance = 1800.f;
		p.fFogMaxOpacity          = 0.06f;
		p.fFogDetailStrength      = 0.05f;
		p.fFogDetailNoiseScale    = 0.00025f;
		p.fFogDetailNoiseSpeed    = 0.010f;
		p.fFogDetailHeightRange   = 650.f;
		p.fFogDetailDistanceStart = 3000.f;
		p.fFogDetailDirectionalScattering = 0.18f;
		p.fMoonIntensity          = 3.1f;
		p.fSkyAmbientIntensity    = 0.301f;
		return p;
	}();

	switch (presetId) {
	case EP_NIGHT:   return NIGHT;
	case EP_DAWN:    return DAWN;
	case EP_SUNSET:  return SUNSET;
	case EP_DEFAULT:
	default:         return DEFAULT;
	}
}

namespace {
	// 석양 시네마틱 카메라 고정 위치 (cm). 플레이어와 무관하게 이 지점에서 노을을 바라봄.
	// ▼ 맵에 맞는 전망 좋은 지점으로 수정하세요.
	const Vector3 SUNSET_CAM_POS{ 50600.f, 500.f, 22000.f };

	// 현재 씬 환경 → 프리셋 스냅샷(페이드 시작값).
	EnvironmentPreset CaptureEnvironment(Scene* pScene)
	{
		EnvironmentPreset p;
		const auto& tone  = pScene->GetToneMappingVolume().GetCommonParameters();
		const auto& grading = pScene->GetToneMappingVolume().GetGradingParameters();
		const auto& bloom = pScene->GetPostProcessingVolume().GetBloomParameters();
		const auto& fx    = pScene->GetPostProcessingVolume().GetScreenFXParameters();
		const auto& cinematicFX = pScene->GetPostProcessingVolume().GetCinematicScreenFXParameters();
		const auto& fog   = pScene->GetPostProcessingVolume().GetFogParameters();

		p.fExposure          = tone.fExposure;
		p.fPostSaturation    = tone.fPostSaturation;
		p.fOutputScale       = tone.fOutputScale;
		p.fGamma             = tone.fGamma;
		p.fGradingStrength   = tone.fGradingStrength;
		p.fTemperature       = grading.fTemperature;
		p.fTint              = grading.fTint;
		p.fGradingContrast   = grading.fContrast;
		p.fGradingSaturation = grading.fSaturation;
		p.fGradingDensity    = grading.fDensity;
		p.v3ShadowTint       = grading.v3ShadowTint;
		p.fShadowWeight      = grading.fShadowWeight;
		p.v3MidtoneTint      = grading.v3MidtoneTint;
		p.fMidtoneWeight     = grading.fMidtoneWeight;
		p.v3HighlightTint    = grading.v3HighlightTint;
		p.fHighlightWeight   = grading.fHighlightWeight;
		p.v3ColorFilter      = grading.v3ColorFilter;
		p.fColorFilterStrength = grading.fColorFilterStrength;
		p.fBlackLift         = grading.fBlackLift;
		p.nEnableAutoExposure = tone.nEnableAutoExposure;
		p.bAffectToneMapperMode = true;
		p.nToneMapperMode    = static_cast<int>(pScene->GetToneMappingVolume().GetCurrentToneMapper());

		p.fBloomThreshold    = bloom.fThreshold;
		p.fBloomIntensity    = bloom.fIntensity;
		p.fBloomRadius       = bloom.fRadius;

		p.fGrainStrength     = fx.fGrainStrength;
		p.fGrainScale        = fx.fGrainScale;
		p.fVignetteStrength  = fx.fVignetteStrength;
		p.fVignetteRadius    = fx.fVignetteRadius;
		p.fVignetteSoftness  = fx.fVignetteSoftness;
		p.fChromaticAberration = cinematicFX.fChromaticAberration;
		p.fHalationStrength    = cinematicFX.fHalationStrength;

		const auto& lightShaft = pScene->GetPostProcessingVolume().GetLightShaftParameters();
		p.bEnableLightShaft    = lightShaft.bEnable;
		p.fLightShaftIntensity = lightShaft.fIntensity;
		p.fLightShaftDensity   = lightShaft.fDensity;
		p.fLightShaftWeight    = lightShaft.fWeight;
		p.fLightShaftExposure  = lightShaft.fExposure;

		p.v4FogColor              = fog.v4FogColor;
		p.fFogStartDistance       = fog.fFogStartDistance;
		p.fFogCutOffDistance      = fog.fFogCutOffDistance;
		p.fFogDistanceDensity     = fog.fFogDistanceDensity;
		p.fFogDistancePower       = fog.fFogDistancePower;
		p.fFogHeightDensity       = fog.fFogHeightDensity;
		p.fFogHeightFalloff       = fog.fFogHeightFalloff;
		p.fFogBaseHeightOffset    = fog.fFogBaseHeightOffset;
		p.fFogHeightStartDistance = fog.fFogHeightStartDistance;
		p.fFogMaxOpacity          = fog.fFogMaxOpacity;
		p.fFogDetailStrength      = fog.fFogDetailStrength;
		p.fFogDetailNoiseScale    = fog.fFogDetailNoiseScale;
		p.fFogDetailNoiseSpeed    = fog.fFogDetailNoiseSpeed;
		p.fFogDetailHeightRange   = fog.fFogDetailHeightRange;
		p.fFogDetailDistanceStart = fog.fFogDetailDistanceStart;
		p.fFogDetailDirectionalScattering = fog.fFogDetailDirectionalScattering;

		p.bAffectTime        = true;
		p.fTimeOfDayHour     = pScene->GetSkybox() ? pScene->GetSkybox()->GetDayNightBlend() * 24.0f : 12.0f;
		p.fMoonIntensity     = pScene->GetSkybox() ? pScene->GetSkybox()->GetMoonIntensity() : 1.2f;
		p.fSkyAmbientIntensity = pScene->GetSkybox() ? pScene->GetSkybox()->GetAmbientIntensity() : 0.08f;

		p.bAffectAmbient     = true;
		p.v4GlobalAmbient    = pScene->GetGlobalAmbient();
		return p;
	}

	// from→to 를 t(0~1)로 보간해 씬에 라이브 적용. (int/즉시값은 호출부에서 처리)
	void ApplyEnvironment(Scene* pScene, const EnvironmentPreset& a, const EnvironmentPreset& b, float t)
	{
		auto& toneMapping = pScene->GetToneMappingVolume();
		auto& tone  = toneMapping.GetCommonParameters();
		auto& grading = toneMapping.GetGradingParameters();
		auto& bloom = pScene->GetPostProcessingVolume().GetBloomParameters();
		auto& fx    = pScene->GetPostProcessingVolume().GetScreenFXParameters();
		auto& cinematicFX = pScene->GetPostProcessingVolume().GetCinematicScreenFXParameters();
		auto& lightShaft = pScene->GetPostProcessingVolume().GetLightShaftParameters();
		auto& fog   = pScene->GetPostProcessingVolume().GetFogParameters();

		tone.fExposure          = std::lerp(a.fExposure,         b.fExposure,         t);
		tone.fPostSaturation    = std::lerp(a.fPostSaturation,   b.fPostSaturation,   t);
		tone.fOutputScale       = std::lerp(a.fOutputScale,      b.fOutputScale,      t);
		tone.fGamma             = std::lerp(a.fGamma,            b.fGamma,            t);
		tone.fGradingStrength   = std::lerp(a.fGradingStrength,  b.fGradingStrength,  t);

		grading.fTemperature       = std::lerp(a.fTemperature,             b.fTemperature,             t);
		grading.fTint              = std::lerp(a.fTint,                    b.fTint,                    t);
		grading.fContrast          = std::lerp(a.fGradingContrast,         b.fGradingContrast,         t);
		grading.fSaturation        = std::lerp(a.fGradingSaturation,       b.fGradingSaturation,       t);
		grading.fDensity           = std::lerp(a.fGradingDensity,          b.fGradingDensity,          t);
		grading.v3ShadowTint       = Vector3::Lerp(a.v3ShadowTint,         b.v3ShadowTint,             t);
		grading.fShadowWeight      = std::lerp(a.fShadowWeight,            b.fShadowWeight,            t);
		grading.v3MidtoneTint      = Vector3::Lerp(a.v3MidtoneTint,        b.v3MidtoneTint,            t);
		grading.fMidtoneWeight     = std::lerp(a.fMidtoneWeight,           b.fMidtoneWeight,           t);
		grading.v3HighlightTint    = Vector3::Lerp(a.v3HighlightTint,      b.v3HighlightTint,          t);
		grading.fHighlightWeight   = std::lerp(a.fHighlightWeight,         b.fHighlightWeight,         t);
		grading.v3ColorFilter      = Vector3::Lerp(a.v3ColorFilter,        b.v3ColorFilter,            t);
		grading.fColorFilterStrength = std::lerp(a.fColorFilterStrength,   b.fColorFilterStrength,     t);
		grading.fBlackLift         = std::lerp(a.fBlackLift,               b.fBlackLift,               t);
		toneMapping.SetDirtyFlag(LUT_DIRTY_FLAG::GRADING, true);

		bloom.fThreshold        = std::lerp(a.fBloomThreshold,   b.fBloomThreshold,   t);
		bloom.fIntensity        = std::lerp(a.fBloomIntensity,   b.fBloomIntensity,   t);
		bloom.fRadius           = std::lerp(a.fBloomRadius,      b.fBloomRadius,      t);

		fx.fGrainStrength       = std::lerp(a.fGrainStrength,    b.fGrainStrength,    t);
		fx.fGrainScale          = std::lerp(a.fGrainScale,       b.fGrainScale,       t);
		fx.fVignetteStrength    = std::lerp(a.fVignetteStrength, b.fVignetteStrength, t);
		fx.fVignetteRadius      = std::lerp(a.fVignetteRadius,   b.fVignetteRadius,   t);
		fx.fVignetteSoftness    = std::lerp(a.fVignetteSoftness, b.fVignetteSoftness, t);
		cinematicFX.fChromaticAberration = std::lerp(a.fChromaticAberration, b.fChromaticAberration, t);
		cinematicFX.fHalationStrength    = std::lerp(a.fHalationStrength,    b.fHalationStrength,    t);

		lightShaft.bEnable      = b.bEnableLightShaft;
		lightShaft.fIntensity   = std::lerp(a.fLightShaftIntensity, b.fLightShaftIntensity, t);
		lightShaft.fDensity     = std::lerp(a.fLightShaftDensity,   b.fLightShaftDensity,   t);
		lightShaft.fWeight      = std::lerp(a.fLightShaftWeight,    b.fLightShaftWeight,    t);
		lightShaft.fExposure    = std::lerp(a.fLightShaftExposure,  b.fLightShaftExposure,  t);

		fog.v4FogColor              = Vector4::Lerp(a.v4FogColor, b.v4FogColor, t);
		fog.fFogStartDistance       = std::lerp(a.fFogStartDistance,       b.fFogStartDistance,       t);
		fog.fFogCutOffDistance      = std::lerp(a.fFogCutOffDistance,      b.fFogCutOffDistance,      t);
		fog.fFogDistanceDensity     = std::lerp(a.fFogDistanceDensity,     b.fFogDistanceDensity,     t);
		fog.fFogDistancePower       = std::lerp(a.fFogDistancePower,       b.fFogDistancePower,       t);
		fog.fFogHeightDensity       = std::lerp(a.fFogHeightDensity,       b.fFogHeightDensity,       t);
		fog.fFogHeightFalloff       = std::lerp(a.fFogHeightFalloff,       b.fFogHeightFalloff,       t);
		fog.fFogBaseHeightOffset    = std::lerp(a.fFogBaseHeightOffset,    b.fFogBaseHeightOffset,    t);
		fog.fFogHeightStartDistance = std::lerp(a.fFogHeightStartDistance, b.fFogHeightStartDistance, t);
		fog.fFogMaxOpacity          = std::lerp(a.fFogMaxOpacity,          b.fFogMaxOpacity,          t);
		fog.fFogDetailStrength      = std::lerp(a.fFogDetailStrength,      b.fFogDetailStrength,      t);
		fog.fFogDetailNoiseScale    = std::lerp(a.fFogDetailNoiseScale,    b.fFogDetailNoiseScale,    t);
		fog.fFogDetailNoiseSpeed    = std::lerp(a.fFogDetailNoiseSpeed,    b.fFogDetailNoiseSpeed,    t);
		fog.fFogDetailHeightRange   = std::lerp(a.fFogDetailHeightRange,   b.fFogDetailHeightRange,   t);
		fog.fFogDetailDistanceStart = std::lerp(a.fFogDetailDistanceStart, b.fFogDetailDistanceStart, t);
		fog.fFogDetailDirectionalScattering = std::lerp(a.fFogDetailDirectionalScattering, b.fFogDetailDirectionalScattering, t);

		if (pScene->GetSkybox()) {
			if (b.bAffectTime) {
				pScene->GetSkybox()->SetTimeOfDayHours(std::lerp(a.fTimeOfDayHour, b.fTimeOfDayHour, t));
			}
			pScene->GetSkybox()->SetMoonIntensity(std::lerp(a.fMoonIntensity, b.fMoonIntensity, t));
			pScene->GetSkybox()->SetAmbientIntensity(std::lerp(a.fSkyAmbientIntensity, b.fSkyAmbientIntensity, t));
		}
		if (b.bAffectAmbient) {
			pScene->SetGlobalAmbient(Vector4::Lerp(a.v4GlobalAmbient, b.v4GlobalAmbient, t));
		}
	}
}

void EnvironmentTransitionEvent::OnEnterEvent(Scene* pScene)
{
	if (!pScene) { m_bFinished = true; return; }

	m_Start        = CaptureEnvironment(pScene);
	m_fTimeElapsed = 0.f;

	// 수동 노출(0)로 가는 룩이면 페이드 내내 exposure lerp가 보이도록 즉시 끔.
	// auto(1)로 복귀하는 경우는 페이드 끝에서 켜야 exposure 보간이 살아있음(OnUpdate 종료시).
	if (m_Target.nEnableAutoExposure == 0) {
		pScene->GetToneMappingVolume().GetCommonParameters().nEnableAutoExposure = 0;
	}

	if (m_Target.bAffectToneMapperMode) {
		const int nMode = std::clamp(
			m_Target.nToneMapperMode,
			static_cast<int>(TONE_MAPPING_MODE::AGX),
			static_cast<int>(TONE_MAPPING_MODE::COUNT) - 1);
		pScene->GetToneMappingVolume().SetCurrentToneMapper(static_cast<TONE_MAPPING_MODE>(nMode));
	}

	// 해질녘 카메라 연출: 시간이 바뀌는 프리셋(bAffectTime)이고 bWatchSun일 때만.
	if (m_Target.bWatchSun && m_Target.bAffectTime) {
		m_bWatchSun = true;
		pScene->PushCinematic(); // 컷씬 동안 입력/좀비 정지

		// 시네마틱 카메라 구성 후 메인 카메라와 교체 (HelicopterCrashEvent와 동일 방식)
		m_pCinematicCamera = std::make_shared<Camera>();
		m_pCinematicCamera->SetViewport(0, 0, WinCore::g_dwClientWidth, WinCore::g_dwClientHeight, 0.f, 1.f);
		m_pCinematicCamera->SetScissorRect(0, 0, WinCore::g_dwClientWidth, WinCore::g_dwClientHeight);
		m_pCinematicCamera->GenerateViewMatrix(XMFLOAT3(0.f, 0.f, 0.f), XMFLOAT3(0.f, 0.f, 1.f), XMFLOAT3(0.f, 1.f, 0.f));
		m_pCinematicCamera->GenerateProjectionMatrix(
			10.f, 1000_m,
			static_cast<float>(WinCore::g_dwClientWidth) / static_cast<float>(WinCore::g_dwClientHeight),
			60.f);
		m_pSavedCamera = pScene->SwapCamera(m_pCinematicCamera);

		// 카메라 위치 = 고정 좌표(플레이어 위치와 무관). 상단 SUNSET_CAM_POS 만 수정.
		m_v3CamPos = SUNSET_CAM_POS;
	}
}

void EnvironmentTransitionEvent::OnUpdateEvent(Scene* pScene)
{
	if (!pScene) { m_bFinished = true; return; }

	m_fTimeElapsed += DT;
	const float t = (m_fDuration > 0.f) ? std::clamp(m_fTimeElapsed / m_fDuration, 0.f, 1.f) : 1.f;

	ApplyEnvironment(pScene, m_Start, m_Target, t);

	// 해질녘 연출: 매 프레임 태양 방향으로 시선을 맞춤(해가 내려가면 카메라도 따라 내려감).
	if (m_bWatchSun && m_pCinematicCamera) {
		Vector3 v3Sun = pScene->GetSkybox() ? pScene->GetSkybox()->GetSunDirection() : Vector3::Up;

		// 태양의 방위(수평 방향)만 추출
		Vector3 v3Azimuth{ v3Sun.x, 0.f, v3Sun.z };
		if (v3Azimuth.LengthSquared() < 1e-4f) v3Azimuth = Vector3::Backward;
		v3Azimuth.Normalize();

		// 태양 고도각을 [0°, 30°]로 클램프 → 지평선 아래(땅속)도, 머리 위(수직)도 안 봄.
		// 해가 높을 땐 30°에 머물다, 지평선으로 내려오는 마지막 구간을 0°까지 따라 내려감.
		const float fSunPitch = std::asin(std::clamp(v3Sun.y, -1.f, 1.f));
		const float fPitch    = std::clamp(fSunPitch, 0.f, XMConvertToRadians(30.f));

		Vector3 v3Look = v3Azimuth * std::cos(fPitch) + Vector3::Up * std::sin(fPitch);
		v3Look.Normalize();

		m_pCinematicCamera->SetPosition(m_v3CamPos);
		m_pCinematicCamera->SetLookTo(v3Look, Vector3::Up);
		m_pCinematicCamera->Update();
	}

	if (t >= 1.f) {
		// 즉시값(자동노출) 최종 확정.
		pScene->GetToneMappingVolume().GetCommonParameters().nEnableAutoExposure = m_Target.nEnableAutoExposure;
		RestoreCamera(pScene); // 해질녘 연출이었다면 원래 카메라로 복구
		m_bFinished = true;
	}
}

void EnvironmentTransitionEvent::RestoreCamera(Scene* pScene)
{
	if (!m_bWatchSun) return;
	if (pScene && m_pSavedCamera) {
		pScene->SwapCamera(m_pSavedCamera);
	}
	if (pScene) pScene->PopCinematic(); // 게임플레이 정지 해제
	m_pSavedCamera.reset();
	m_pCinematicCamera.reset();
	m_bWatchSun = false;
}

namespace {
	// 헬기 추락 궤적 (플레이어 기준, cm)
	const float HELI_AIR_HEIGHT   = 16000.f; // 충돌점 대비 출발 고도
	const float HELI_AHEAD_DIST   = 30000.f; // 충돌점: 플레이어 정면 거리(멀리서 추락)
	const float HELI_START_AHEAD  = 9000.f;  // 출발점: 충돌점보다 더 먼 정면 거리(비스듬히 진입)
	const float HELI_SIDE_OFFSET  = 9000.f;  // 출발점 측면 오프셋(대각선 낙하)
	const float HELI_GROUND_DROP  = 200.f;   // 충돌점을 플레이어 발밑보다 약간 아래로
	// 3인칭 시점 유지: 스왑 직전 플레이어 카메라 위치를 그대로 두고 헬기만 바라봄.
	const float CAM_FOLLOW_SPEED  = 3.5f;   // 시선 추적 속도(작을수록 지연 큼 → "따라붙는" 느낌)
	// 플레이어 yaw 기준 3인칭 어깨 시점 오프셋 (ThirdPersonCamera FreeMode/AimMode 참고)
	const float CAM_BACK_DIST     = 3.0_m;  // 플레이어 뒤
	const float CAM_HEIGHT        = 1.5_m;  // 플레이어 위(어깨 높이)
	const float CAM_SHOULDER      = 30_cm;  // 우측 어깨 오프셋
	// ▼ 헬기 모델 교체 지점 — 진짜 헬기 모델 나오면 이 두 줄만 바꾸면 됨.
	const char*   HELI_MODEL_NAME = "Gunship";          // 헬기 모델 이름
	const Vector3 HELI_SCALE{ 1.f, 1.f, 1.f };         // placeholder 큐브용 스케일. 실제 모델은 보통 {1,1,1}
	// 모델이 옆으로 누운 채 익스포트됨 → 세우는 보정 회전 (Pitch=X, Yaw=Y, Roll=Z, 도 단위).
	// 기수 정렬(mtxRot) 전에 적용돼 모델 로컬축을 엔진축(forward=+Z, up=+Y)에 맞춘다.
	// 방향이 안 맞으면 이 세 값만 조정 (예: 90 / -90 / 180).
	const Vector3 HELI_MODEL_FIX_EULER{ 0.f, 0.f, 90.f };

	// 경로점들을 Catmull-Rom으로 보간해 t01(0~1) 위치 샘플 (CinematicCameraEvent와 동일 방식)
	Vector3 SampleCatmullRom(const std::vector<Vector3>& pts, float t01)
	{
		const size_t nCount = pts.size();
		if (nCount == 0) return Vector3::Zero;
		if (nCount == 1) return pts[0];

		t01 = std::clamp(t01, 0.f, 1.f);
		const size_t nSeg = nCount - 1;
		const float  fSegF = t01 * static_cast<float>(nSeg);
		const size_t i1 = std::min(static_cast<size_t>(std::floor(fSegF)), nSeg - 1);
		const float  fLocalT = fSegF - static_cast<float>(i1);
		const size_t i0 = (i1 == 0) ? i1 : i1 - 1;
		const size_t i2 = i1 + 1;
		const size_t i3 = std::min(i2 + 1, nCount - 1);
		return Vector3::CatmullRom(pts[i0], pts[i1], pts[i2], pts[i3], fLocalT);
	}
}

void HelicopterCrashEvent::OnEnterEvent(Scene* pScene)
{
	if (!pScene) { m_bFinished = true; return; }

	m_fTimeElapsed = 0.f;
	m_bExploded    = false;

	pScene->PushCinematic(); // 컷씬 동안 입력/좀비 정지
	m_bCinematicPushed = true;

	// 시네마틱 카메라 구성 후 현재 카메라와 교체
	m_pCinematicCamera = std::make_shared<Camera>();
	m_pCinematicCamera->SetViewport(0, 0, WinCore::g_dwClientWidth, WinCore::g_dwClientHeight, 0.f, 1.f);
	m_pCinematicCamera->SetScissorRect(0, 0, WinCore::g_dwClientWidth, WinCore::g_dwClientHeight);
	m_pCinematicCamera->GenerateViewMatrix(XMFLOAT3(0.f, 0.f, 0.f), XMFLOAT3(0.f, 0.f, 1.f), XMFLOAT3(0.f, 1.f, 0.f));
	m_pCinematicCamera->GenerateProjectionMatrix(
		10.f, 10000_m, 
		static_cast<float>(WinCore::g_dwClientWidth) / static_cast<float>(WinCore::g_dwClientHeight),
		60.f);
	m_pSavedCamera = pScene->SwapCamera(m_pCinematicCamera);

	// 플레이어 위치 스냅샷(컷씬 중 입력 영향 차단). 카메라는 매 프레임 이 위치를 기준으로
	// "플레이어→헬기" 방향 뒤쪽에 재배치된다(OnUpdateEvent 참고).
	if (const auto& pPlayer = pScene->GetPlayer()) {
		m_v3PlayerPos = pPlayer->GetTransform()->GetPosition();
	}
	else {
		m_v3PlayerPos = m_pSavedCamera ? m_pSavedCamera->GetPosition() : Vector3::Zero;
	}

	// 언리얼에서 내보낸 비행 경로(2점 이상)가 있으면 그 경로를 보간해 비행.
	// 착륙(구조) 모드는 별도 경로(ArrivePath), 추락 모드는 추락 경로(HeliPath) 사용.
	m_v3PathPoints = m_bLandMode ? pScene->GetHeliArrivePath() : pScene->GetHeliPath();
	OutputDebugStringA(std::format("[HeliCrash] OnEnter path points = {} ({})\n",
		m_v3PathPoints.size(),
		m_v3PathPoints.size() >= 2 ? "USING PATH" : "FALLBACK (player-relative)").c_str());
	if (m_v3PathPoints.size() >= 2) {
		m_v3HeliStart = m_v3PathPoints.front();
		m_v3HeliEnd   = m_v3PathPoints.back(); // 폭발 위치 = 경로 끝점
	}
	else {
		// 폴백: 경로 파일이 없으면 플레이어 정면 기준 직선 궤적 산출.
		Vector3 v3PlayerPos = Vector3::Zero;
		Vector3 v3Forward   = Vector3::Backward;
		if (const auto& pPlayer = pScene->GetPlayer()) {
			const auto& pTransform = pPlayer->GetTransform();
			v3PlayerPos = pTransform->GetPosition();
			v3Forward   = pTransform->GetLook();
		}
		v3Forward.y = 0.f; // 수평 정면
		if (v3Forward.LengthSquared() < 1e-4f) v3Forward = Vector3::Backward;
		v3Forward.Normalize();
		Vector3 v3Right = Vector3::Up.Cross(v3Forward);
		v3Right.Normalize();

		// 충돌점 = 플레이어 정면 지면, 출발점 = 더 먼 정면 상공 + 측면 오프셋(대각선 진입)
		m_v3HeliEnd   = v3PlayerPos + v3Forward * HELI_AHEAD_DIST + Vector3(0.f, -HELI_GROUND_DROP, 0.f);
		m_v3HeliStart = m_v3HeliEnd
			+ Vector3(0.f, HELI_AIR_HEIGHT, 0.f)
			+ v3Forward * HELI_START_AHEAD
			+ v3Right   * HELI_SIDE_OFFSET;
	}

	m_v3LookAt = m_v3HeliStart; // 시작 시선 = 경로 시작점(상공의 헬기)

	// 헬기 오브젝트 — Gunship 모델 + 로터 회전을 캡슐화한 HelicopterObject 사용.
	// (모델 로드/방향보정/로터 애니는 HelicopterObject::Initialize/Update가 담당)
	m_pHeli = std::make_shared<HelicopterObject>();
	m_pHeli->Initialize();
	m_pHeli->GetTransform()->SetPosition(m_v3HeliStart);
	m_pHeli->GetTransform()->Update();
	m_pHeli->PlaySound(m_bLandMode ? "helicopter_landing" : "helicopter_crash");
	// 컷씬 중 World 정지(bFrozen)라 위치/로터는 OnUpdateEvent에서 직접 갱신한다.
	pScene->AddObject(m_pHeli);
}

void HelicopterCrashEvent::OnUpdateEvent(Scene* pScene)
{
	if (!pScene || !m_pHeli || !m_pCinematicCamera) { Finish(pScene); return; }

	m_fTimeElapsed += DT;
	const float t = std::clamp(m_fTimeElapsed / m_fDuration, 0.f, 1.f);

	// 하강 가속(ease-in): 뒤로 갈수록 빨라지는 추락감
	const float fFall = t * t;

	// 위치 산출: 경로점이 있으면 Catmull-Rom 보간, 없으면 직선 폴백.
	// 진행 방향(v3Travel)으로 기수를 정렬해 경로를 따라 "날아가는" 모습.
	Vector3 v3HeliPos;
	Vector3 v3Travel;
	if (m_v3PathPoints.size() >= 2) {
		v3HeliPos = SampleCatmullRom(m_v3PathPoints, fFall);
		const Vector3 v3Ahead = SampleCatmullRom(m_v3PathPoints, std::min(fFall + 0.02f, 1.f));
		v3Travel = v3Ahead - v3HeliPos;
	}
	else {
		v3HeliPos = Vector3::Lerp(m_v3HeliStart, m_v3HeliEnd, fFall);
		v3Travel  = m_v3HeliEnd - m_v3HeliStart;
	}

	// 기수를 진행 방향으로 정렬 (진행 벡터가 너무 작으면 회전 생략).
	// CreateWorld는 forward의 반대를 +Z로 잡으므로 -v3Travel을 넣어야 기수가 진행 방향을 본다.
	// 착륙(구조) 모드는 경로 기수 정렬을 끄고 고정 방향(모델 기본 방향) 유지.
	Matrix mtxRot = Matrix::Identity;
	if (!m_bLandMode && v3Travel.LengthSquared() > 1e-4f) {
		v3Travel.Normalize();
		Vector3 v3Up = Vector3::Up;
		if (fabsf(v3Travel.Dot(v3Up)) > 0.99f) v3Up = Vector3::Backward; // 수직 비행 시 up 특이점 회피
		mtxRot = Matrix::CreateWorld(Vector3::Zero, -v3Travel, v3Up); // 기수 = 진행 방향
	}

	// 모델 방향 보정은 HelicopterObject가 자체 처리(자식 모델 -90°)하므로 여기선 기수 정렬만.
	const Matrix mtxWorld =
		Matrix::CreateScale(HELI_SCALE) *
		mtxRot *
		Matrix::CreateTranslation(v3HeliPos);
	m_pHeli->GetTransform()->SetWorldMatrix(mtxWorld);
	m_pHeli->Update();      // 로터 회전 (컷씬 정지 중이라 직접 호출)
	m_pHeli->PostUpdate();  // 루트 → 자식 world 행렬 전파

	// 카메라를 매 프레임 "플레이어→헬기" 방향 기준으로 플레이어 뒤에 배치.
	// 헬기가 이동하면 카메라도 그 반대편(등 뒤)으로 돌며 항상 어깨 너머로 추락을 좇는다.
	Vector3 v3ToHeli = v3HeliPos - m_v3PlayerPos;
	v3ToHeli.y = 0.f; // 수평 방향만
	if (v3ToHeli.LengthSquared() < 1e-4f) v3ToHeli = Vector3::Backward;
	v3ToHeli.Normalize();
	Vector3 v3Right = Vector3::Up.Cross(v3ToHeli); // 우측(어깨 방향)
	v3Right.Normalize();

	const Vector3 v3CamPos = m_v3PlayerPos
		- v3ToHeli * CAM_BACK_DIST       // 플레이어 뒤로
		+ Vector3(0.f, CAM_HEIGHT, 0.f)  // 위로(어깨 높이)
		+ v3Right * CAM_SHOULDER;        // 우측 어깨 오프셋

	// 시선: 헬기를 부드럽게 추적 (떨어지는 헬기를 눈으로 좇는 느낌)
	const float fDamp = 1.f - expf(-CAM_FOLLOW_SPEED * DT);
	m_v3LookAt = Vector3::Lerp(m_v3LookAt, v3HeliPos, fDamp);

	Vector3 v3Look = m_v3LookAt - v3CamPos;
	if (v3Look.LengthSquared() < 0.0001f) v3Look = Vector3::Backward;
	v3Look.Normalize();
	m_pCinematicCamera->SetPosition(v3CamPos);
	m_pCinematicCamera->SetLookTo(v3Look, Vector3::Up);
	m_pCinematicCamera->Update();

	// 종료 처리 (t>=1):
	//  - 착륙 모드: 폭발 없이 착륙 지점 기록 → 탈출 시퀀스가 이어받음 (헬기는 월드에 남음)
	//  - 추락 모드: 폭발 파티클 + 사운드
	if (!m_bExploded && t >= 1.f) {
		if (m_bLandMode) {
			m_v3ExtractionPos = v3HeliPos;
			m_bLanded = true;
			m_pHeli->PlaySound("helicopter_idle");
		}
		else {
			SpawnExplosion(m_v3HeliEnd);
			if (m_pHeli) m_pHeli->SetRotorActive(false); // 추락 후 잔해 → 로터 정지
			// 잔해 화재 — 차량 화재와 동일한 불 파티클을 추락 지점에 상시 유지
			if (const auto& pSeq = pScene->GetEventSequence())
				pSeq->AddEvent(std::make_shared<CrashSiteFireEvent>(m_v3HeliEnd));
		}
		m_bExploded = true;
		// 헬기는 OnEnter에서 이미 AddObject로 등록됨 → 그대로 World에 남는다.
		Finish(pScene);
	}
}

void HelicopterCrashEvent::SpawnExplosion(const Vector3& v3Pos)
{
	ParticleEffectSpawnDesc desc;
	desc.v3Position  = v3Pos;
	desc.v3Direction = Vector3::Up;
	desc.v3Normal    = Vector3::Up;
	desc.mtxWorld    = Matrix::CreateWorld(desc.v3Position, desc.v3Direction, desc.v3Normal);
	PARTICLE->Spawn<ExplosionEffect>(desc);

	SOUND->PlayAt("helicopter_explosion", v3Pos);
}

void HelicopterCrashEvent::Finish(Scene* pScene)
{
	if (pScene) {
		if (m_pSavedCamera) {
			pScene->SwapCamera(m_pSavedCamera);
		}
		pScene->ClearCinematicProp();
		if (m_bCinematicPushed) { pScene->PopCinematic(); m_bCinematicPushed = false; } // 정지 해제
	}
	m_pSavedCamera.reset();
	// 착륙 모드는 헬기를 유지(출발 컷씬에서 그대로 재사용). 추락 모드만 잔해 핸들 해제.
	if (!m_bLandMode) m_pHeli.reset();
	m_bFinished = true;
}

// ── 탈출 출발 컷씬 ──────────────────────────────────────────────────────────
void HelicopterDepartEvent::OnEnterEvent(Scene* pScene)
{
	if (!pScene || !m_pHeli || m_v3PathPoints.size() < 2) { m_bFinished = true; return; }

	m_fTimeElapsed = 0.f;
	m_pHeli->PlaySound("helicopter_takeoff");

	pScene->PushCinematic();          // 입력/월드 정지
	m_bCinematicPushed = true;
	pScene->SetHideCharacters(true);  // 전 플레이어 숨김

	// 하늘 고정 카메라 — 착륙점 기준 위/옆에 한 번 배치하고 그대로 고정(추적 X).
	const Vector3 v3Land = m_v3PathPoints.front();
	const Vector3 v3CamPos = v3Land + Vector3(3000.f, 5000.f, 3000.f); // 옆+상공 (튜닝 가능)
	const Vector3 v3LookAt = v3Land + Vector3(0.f, 2000.f, 0.f);       // 상승 구간 중앙을 바라봄

	m_pCinematicCamera = std::make_shared<Camera>();
	m_pCinematicCamera->SetViewport(0, 0, WinCore::g_dwClientWidth, WinCore::g_dwClientHeight, 0.f, 1.f);
	m_pCinematicCamera->SetScissorRect(0, 0, WinCore::g_dwClientWidth, WinCore::g_dwClientHeight);
	m_pCinematicCamera->GenerateViewMatrix(XMFLOAT3(0.f, 0.f, 0.f), XMFLOAT3(0.f, 0.f, 1.f), XMFLOAT3(0.f, 1.f, 0.f));
	m_pCinematicCamera->GenerateProjectionMatrix(
		10.f, 10000_m,
		static_cast<float>(WinCore::g_dwClientWidth) / static_cast<float>(WinCore::g_dwClientHeight), 60.f);

	Vector3 v3Look = v3LookAt - v3CamPos;
	if (v3Look.LengthSquared() < 0.0001f) v3Look = Vector3::Backward;
	v3Look.Normalize();
	m_pCinematicCamera->SetPosition(v3CamPos);
	m_pCinematicCamera->SetLookTo(v3Look, Vector3::Up);
	m_pCinematicCamera->Update();

	m_pSavedCamera = pScene->SwapCamera(m_pCinematicCamera);
}

void HelicopterDepartEvent::OnUpdateEvent(Scene* pScene)
{
	if (!pScene || !m_pHeli) { Finish(pScene); return; }

	m_fTimeElapsed += DT;
	const float t = std::clamp(m_fTimeElapsed / m_fDuration, 0.f, 1.f);

	// 상승: ease-in(t²)로 서서히 가속하며 떠오름. 경로 [0]착륙점 → [last]상공.
	const float fRise = t * t;
	const Vector3 v3HeliPos = SampleCatmullRom(m_v3PathPoints, fRise);

	// 경로 기수 정렬은 끔 — 착륙/상승 모두 고정 방향(모델 기본 방향) 유지.
	const Matrix mtxWorld =
		Matrix::CreateScale(HELI_SCALE) * Matrix::CreateTranslation(v3HeliPos);
	m_pHeli->GetTransform()->SetWorldMatrix(mtxWorld);
	m_pHeli->Update();      // 로터 회전 (컷씬 정지 중이라 직접 호출)
	m_pHeli->PostUpdate();

	// 카메라는 고정(매 프레임 갱신 안 함). 도달 시 종료.
	if (t >= 1.f) Finish(pScene);
}

void HelicopterDepartEvent::Finish(Scene* pScene)
{
	if (pScene) {
		if (m_pSavedCamera) pScene->SwapCamera(m_pSavedCamera);
		if (m_bCinematicPushed) { pScene->PopCinematic(); m_bCinematicPushed = false; }
		// 플레이어 숨김은 유지하지 않음(게임 종료 화면으로 넘어가므로 복구). 필요 시 GameScene이 처리.
		pScene->SetHideCharacters(false);
	}
	m_pSavedCamera.reset();
	m_pHeli.reset();
	m_bFinished = true;
}

void CinematicCameraEvent::Initialize(Scene* pScene)
{
	m_fnBeginTrigger = [&]() ->bool {
		return INPUT->GetButtonDown('P');
	};
	
	m_fnEndTrigger = [&]() -> bool {
		return m_fTimeElapsed >= m_fTotalPlayTime;
	};

	m_v3ControlPoints.reserve(10);
	for (uint32 i = 0; i < 10; ++i) {
		float x = RandomGenerator::GenerateRandomFloatInRange(2000.f, 8000.f);
		float y = RandomGenerator::GenerateRandomFloatInRange(500, 1000.f);
		float z = RandomGenerator::GenerateRandomFloatInRange(0.f, 10000.f);
		m_v3ControlPoints.emplace_back(x, y, z);
	}

	m_pCinematicCamera = std::make_shared<Camera>();
	m_pCinematicCamera->SetViewport(0, 0, WinCore::g_dwClientWidth, WinCore::g_dwClientHeight, 0.f, 1.f);
	m_pCinematicCamera->SetScissorRect(0, 0, WinCore::g_dwClientWidth, WinCore::g_dwClientHeight);
	m_pCinematicCamera->GenerateViewMatrix(
		XMFLOAT3(0.f, 0.f, -15.f),
		XMFLOAT3(0.f, 0.f, 1.f),
		XMFLOAT3(0.f, 1.f, 0.f)
	);
	m_pCinematicCamera->GenerateProjectionMatrix(
		10.f,
		300_m,
		static_cast<float>(WinCore::g_dwClientWidth) / static_cast<float>(WinCore::g_dwClientHeight),
		60.0f
	);
}

void CinematicCameraEvent::OnEnterEvent(Scene* pScene)
{
	m_pCameraSwapped = pScene->SwapCamera(m_pCinematicCamera);
	m_v3ControlPoints.back() = m_pCameraSwapped->GetPosition();
}

void CinematicCameraEvent::OnUpdateEvent(Scene* pScene)
{
	if (!pScene || !m_pCinematicCamera) {
		return;
	}

	if (m_v3ControlPoints.size() < 2) {
		return;
	}

	m_fTimeElapsed += DT;

	const float t01 = std::clamp(m_fTimeElapsed / m_fTotalPlayTime, 0.0f, 1.0f);

	const size_t pointCount = m_v3ControlPoints.size();
	const size_t segmentCount = pointCount - 1;

	const float segmentFloat = t01 * static_cast<float>(segmentCount);
	size_t i1 = static_cast<size_t>(std::floor(segmentFloat));
	i1 = std::min(i1, segmentCount - 1);

	const float localT = segmentFloat - static_cast<float>(i1);

	const size_t i0 = (i1 == 0) ? i1 : i1 - 1;
	const size_t i2 = i1 + 1;
	const size_t i3 = std::min(i2 + 1, pointCount - 1);

	const Vector3 camPos = Vector3::CatmullRom(
		m_v3ControlPoints[i0],
		m_v3ControlPoints[i1],
		m_v3ControlPoints[i2],
		m_v3ControlPoints[i3],
		localT
	);

	const float nextT01 = std::clamp(
		(m_fTimeElapsed + 0.05f) / m_fTotalPlayTime,
		0.0f,
		1.0f
	);

	const float nextSegmentFloat = nextT01 * static_cast<float>(segmentCount);

	size_t nextI1 = static_cast<size_t>(std::floor(nextSegmentFloat));
	nextI1 = std::min(nextI1, segmentCount - 1);

	const float nextLocalT = nextSegmentFloat - static_cast<float>(nextI1);

	const size_t nextI0 = (nextI1 == 0) ? nextI1 : nextI1 - 1;
	const size_t nextI2 = nextI1 + 1;
	const size_t nextI3 = std::min(nextI2 + 1, pointCount - 1);

	const Vector3 nextCamPos = Vector3::CatmullRom(
		m_v3ControlPoints[nextI0],
		m_v3ControlPoints[nextI1],
		m_v3ControlPoints[nextI2],
		m_v3ControlPoints[nextI3],
		nextLocalT
	);

	Vector3 look = nextCamPos - camPos;

	if (look.LengthSquared() < 0.0001f) {
		look = Vector3::Backward;
	}

	look.Normalize();

	m_pCinematicCamera->SetPosition(camPos);
	m_pCinematicCamera->SetLookTo(look, Vector3::Up);
	m_pCinematicCamera->Update();
}

void CinematicCameraEvent::OnLeaveEvent(Scene* pScene)
{
	if (!pScene || !m_pCameraSwapped) {
		return;
	}

	pScene->SwapCamera(m_pCameraSwapped);

	m_pCameraSwapped.reset();
	m_fTimeElapsed = 0.0f;
}

void CrashSiteFireEvent::OnUpdateEvent(Scene* pScene)
{
	// 이펙트 종료 직전에만 겹쳐 재소환해 불의 연속성은 유지하고 중복 파티클은 줄인다.
	constexpr float FIRE_EFFECT_ESTIMATED_DURATION = 5.4f;
	constexpr float FIRE_EFFECT_RESPAWN_LEAD_TIME = 1.6f;
	constexpr float FIRE_EFFECT_RESPAWN_TIME = FIRE_EFFECT_ESTIMATED_DURATION - FIRE_EFFECT_RESPAWN_LEAD_TIME;

	m_fElapsed += DT;

	if (m_pFireEffect && m_pFireEffect->IsPlaying() && !m_pFireEffect->IsDead() && m_fElapsed < FIRE_EFFECT_RESPAWN_TIME) {
		return;
	}

	ParticleEffectSpawnDesc desc{};
	desc.v3Direction = Vector3::Up;
	desc.v3Normal = Vector3::Up;
	desc.v3Position = m_v3Pos;
	desc.mtxWorld = Matrix::CreateWorld(desc.v3Position, desc.v3Direction, desc.v3Normal);
	m_pFireEffect = PARTICLE->Spawn<CrashFireEffect>(desc);
	m_fElapsed = 0.f;
}

void FireEvent::Initialize(Scene* pScene)
{
	m_v3FirePos.clear();
	m_pFireEffects.clear();
	m_fFireEffectElapsed.clear();

	const auto& pLights = pScene->GetLightsInScene();
	for (const auto& pLight : pLights) {
		if (auto point = std::dynamic_pointer_cast<PointLight>(pLight)) {
			m_v3FirePos.push_back(pLight->m_v3Position);
		}
	}

	m_pFireEffects.resize(m_v3FirePos.size());
	m_fFireEffectElapsed.resize(m_v3FirePos.size(), 0.0f);
}

void FireEvent::OnUpdateEvent(Scene* pScene)
{
	constexpr float FIRE_EFFECT_ESTIMATED_DURATION = 5.4f;
	constexpr float FIRE_EFFECT_RESPAWN_LEAD_TIME = 1.6f;
	constexpr float FIRE_EFFECT_RESPAWN_TIME = FIRE_EFFECT_ESTIMATED_DURATION - FIRE_EFFECT_RESPAWN_LEAD_TIME;

	for (size_t i = 0; i < m_v3FirePos.size(); ++i) {
		const auto& pEffect = m_pFireEffects[i];
		m_fFireEffectElapsed[i] += DT;

		if (pEffect && pEffect->IsPlaying() && !pEffect->IsDead() && m_fFireEffectElapsed[i] < FIRE_EFFECT_RESPAWN_TIME) {
			continue;
		}

		ParticleEffectSpawnDesc desc{};
		desc.v3Direction = Vector3::Up;
		desc.v3Normal = Vector3::Up;
		desc.v3Position = m_v3FirePos[i];
		desc.mtxWorld = Matrix::CreateWorld(desc.v3Position, desc.v3Direction, desc.v3Normal);
		m_pFireEffects[i] = PARTICLE->Spawn<FireEffect>(desc);
		m_fFireEffectElapsed[i] = 0.0f;
	}
}
