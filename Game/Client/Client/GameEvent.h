#pragma once


/*
	Event 종류?
	1. 시간 변화 -> 성공
	2. Cinematic Camera 의 이동 (컷씬 재생용) -> 성공 (나중에 CatmullRom -> Hermite 기반으로 변경)
	3. 특정 오브젝트의 소환/이동 (컷씬 재생용)
	4. 파티클 소환 -> 성공
	5. 파라미터 변경
		5-1. Tone
		5-2. Fx (Grading, Vignette)
		5-3. Skybox
		5-4. 더?
*/

class CrashDebris;
class FireEffect;
class CrashFireEffect;
class HelicopterObject;

interface IGameEvent abstract {
public:
	virtual void Initialize(Scene* pScene) {};
	virtual void Update(Scene* pScene) = 0;

	virtual void OnEnterEvent(Scene* pScene) {};
	virtual void OnUpdateEvent(Scene* pScene) {};
	virtual void OnLeaveEvent(Scene* pScene) {};

	virtual void ShowDebugOptions() { };

	// true 반환 시 EventSequence가 다음 Update에서 제거 (1회성/완료 이벤트용)
	virtual bool IsFinished() const { return false; }

protected:
	bool m_bIsTriggered = false;

};

// 1회성 이벤트: 첫 Update에서 OnEnter 1회 실행 후 매 프레임 OnUpdate.
// OnUpdate에서 m_bFinished=true 로 마크하면 EventSequence가 자동 제거.
// 서버 트리거로 런타임에 AddEvent 되는 이벤트에 사용 (Initialize 의존 없음).
interface IOneShotEvent abstract : public IGameEvent {
public:
	virtual void Update(Scene* pScene) override {
		if (!m_bIsTriggered) {
			m_bIsTriggered = true;
			OnEnterEvent(pScene);
		}
		OnUpdateEvent(pScene);
	}
	virtual bool IsFinished() const override { return m_bFinished; }

protected:
	bool m_bFinished = false;
};

interface ILoopEvent abstract : public IGameEvent {
public:
	virtual void Update(Scene* pScene) override;

};

interface IIntervalEvent abstract : public IGameEvent {
public:
	virtual void Update(Scene* pScene) override;

protected:
	float m_fInterval = 0.f;
	float m_fTimeElapsed = 0.f;
};

interface ITriggerEvent abstract : public IGameEvent {
public:
	virtual void Update(Scene* pScene) override;

	void SetBeginTrigger(const std::function<bool()>& fnTrigger) { m_fnBeginTrigger = fnTrigger; }
	void SetEndTrigger(const std::function<bool()>& fnTrigger) { m_fnEndTrigger= fnTrigger; }

protected:
	std::function<bool()> m_fnBeginTrigger;
	std::function<bool()> m_fnEndTrigger;
};

// Test
class TimeForwardEvent : public ILoopEvent {
public:
	virtual void OnUpdateEvent(Scene* pScene) override;

private:
	float m_fTime = 0.f;

};

// Test
class BleedEvent : public IIntervalEvent {
public:
	virtual void Initialize(Scene* pScene) override;
	virtual void OnUpdateEvent(Scene* pScene) override;

private:

};

// Test
class FireEvent : public ILoopEvent {
public:
	virtual void Initialize(Scene* pScene) override;
	virtual void OnUpdateEvent(Scene* pScene) override;

private:
	std::vector<Vector3> m_v3FirePos;
	std::vector<std::shared_ptr<FireEffect>> m_pFireEffects;
	std::vector<float> m_fFireEffectElapsed;
};

// 추락 잔해 화재 — 지정 위치에 차량 화재(FireEvent)와 동일한 리스폰 루프로
// 대형 불 파티클(CrashFireEffect)을 유지. 헬기 추락 폭발 직후 HelicopterCrashEvent가 띄운다.
class CrashSiteFireEvent : public ILoopEvent {
public:
	CrashSiteFireEvent(const Vector3& v3Pos) : m_v3Pos{ v3Pos } {}
	virtual void OnUpdateEvent(Scene* pScene) override;

private:
	Vector3 m_v3Pos;
	std::shared_ptr<CrashFireEffect> m_pFireEffect;
	float m_fElapsed = 0.f;
};

// 폭발 연출 (서버 GE_EXPLOSION). 지정 위치에 파티클 + 3D 사운드 1회 재생 후 종료.
class ExplosionEvent : public IOneShotEvent {
public:
	ExplosionEvent(const Vector3& v3Pos) : m_v3Pos{ v3Pos } {}
	virtual void OnEnterEvent(Scene* pScene) override;

private:
	Vector3 m_v3Pos;
};


// ── 환경 프리셋 틀 ────────────────────────────────────────────────────────────
// 한 이벤트로 화면 환경 전체(톤매핑 + 포스트FX + 안개 + 스카이박스 시간 + 글로벌 앰비언트)를
// 한꺼번에 목표값으로 페이드한다. "룩"은 더 이상 이벤트 클래스가 아니라 EnvironmentPreset 데이터다.
//   - 새 룩 추가 = GetEnvironmentPreset() 테이블에 구조체 항목 하나 추가 (+ protocol.h enum)
//   - 서버는 EnvironmentPresetId 만 보내고, 실제 값/페이드는 전부 여기서 처리
struct EnvironmentPreset {
	// 톤매핑 (라이브 반영되는 Common 값들)
	float fExposure          = 1.0f;
	float fPostSaturation    = 1.0f;
	float fOutputScale       = 1.0f;
	float fGamma             = 2.2f;
	float fGradingStrength   = 0.0f;
	float fTemperature       = 0.0f;
	float fTint              = 0.0f;
	float fGradingContrast   = 1.0f;
	float fGradingSaturation = 1.0f;
	float fGradingDensity    = 0.0f;
	Vector3 v3ShadowTint     { 1.0f, 1.0f, 1.0f };
	float fShadowWeight      = 0.0f;
	Vector3 v3MidtoneTint    { 1.0f, 1.0f, 1.0f };
	float fMidtoneWeight     = 0.0f;
	Vector3 v3HighlightTint  { 1.0f, 1.0f, 1.0f };
	float fHighlightWeight   = 0.0f;
	Vector3 v3ColorFilter    { 1.0f, 1.0f, 1.0f };
	float fColorFilterStrength = 0.0f;
	float fBlackLift         = 0.0f;
	int   nEnableAutoExposure = 1;     // 즉시 적용(lerp 안 함). 0 = 수동 노출(룩 연출용)
	bool  bAffectToneMapperMode = false;
	int   nToneMapperMode    = 1;     // 0=AgX, 1=ACES, 2=UC2, 3=GT. 모드는 이산값이라 lerp 대신 전환 시작 시 적용.

	// 블룸
	float fBloomThreshold    = 1.0f;
	float fBloomIntensity    = 0.6f;
	float fBloomRadius       = 1.0f;

	// 스크린 FX
	float fGrainStrength     = 0.015f;
	float fGrainScale        = 1.0f;
	float fVignetteStrength  = 0.25f;
	float fVignetteRadius    = 0.75f;
	float fVignetteSoftness  = 0.45f;

	// 라이트 샤프트
	bool  bEnableLightShaft    = true;
	float fLightShaftIntensity = 0.65f;
	float fLightShaftDensity   = 0.85f;
	float fLightShaftWeight    = 0.08f;
	float fLightShaftExposure  = 1.0f;

	// 안개
	Vector4 v4FogColor       { 0.62f, 0.68f, 0.72f, 1.0f };
	float   fFogStartDistance       = 5.0f;
	float   fFogCutOffDistance      = 0.0f;
	float   fFogDistanceDensity     = 0.0035f;
	float   fFogDistancePower       = 1.0f;
	float   fFogHeightDensity       = 0.02f;
	float   fFogHeightFalloff       = 0.06f;
	float   fFogBaseHeightOffset    = 0.0f;
	float   fFogHeightStartDistance = 0.0f;
	float   fFogMaxOpacity          = 0.0f;

	// 스카이박스 시간 (0~24 hour). bAffectTime=false 면 시간은 건드리지 않음.
	bool  bAffectTime        = false;
	float fTimeOfDayHour     = 12.0f;
	float fMoonIntensity     = 1.2f;
	float fSkyAmbientIntensity = 0.08f;

	// true 이고 bAffectTime 일 때: 전환 동안 시네마틱 카메라가 태양을 바라봄(해질녘 연출).
	// 연출 끝나면 원래 카메라로 복구. (플레이어 입력은 전환 동안 카메라에 미반영)
	bool  bWatchSun          = false;

	// 글로벌 앰비언트. bAffectAmbient=false 면 앰비언트는 건드리지 않음.
	bool    bAffectAmbient   = false;
	Vector4 v4GlobalAmbient  { 0.1f, 0.1f, 0.1f, 1.0f };
};

// presetId(EnvironmentPresetId) → 프리셋 값. 정의는 GameEvent.cpp.
const EnvironmentPreset& GetEnvironmentPreset(int presetId);

// 환경 전체를 현재값 → 목표 프리셋으로 fDuration 동안 페이드하는 범용 이벤트.
// 서버 GE_ENVIRONMENT 가 이걸 띄운다. (런타임 AddEvent — Initialize 미사용)
class EnvironmentTransitionEvent : public IOneShotEvent {
public:
	EnvironmentTransitionEvent(const EnvironmentPreset& target, float fDuration)
		: m_Target{ target }, m_fDuration{ fDuration } {}
	virtual void OnEnterEvent(Scene* pScene) override;
	virtual void OnUpdateEvent(Scene* pScene) override;

private:
	void RestoreCamera(Scene* pScene); // 해질녘 연출 종료 시 원래 카메라 복구

private:
	EnvironmentPreset m_Target;
	EnvironmentPreset m_Start{};   // OnEnter서 현재 환경 스냅샷
	float m_fDuration;
	float m_fTimeElapsed = 0.f;

	// 해질녘 카메라 연출(bWatchSun) 전용
	bool                    m_bWatchSun = false;
	Vector3                 m_v3CamPos{};        // OnEnter서 고정한 카메라 위치(플레이어 눈높이)
	std::shared_ptr<Camera> m_pCinematicCamera;
	std::shared_ptr<Camera> m_pSavedCamera;
};

// 헬기 추락 컷씬 (서버 GE_HELICOPTER_CRASH). 시네마틱 카메라로 전환 후
// 임시 헬기 오브젝트(SM_Cube placeholder)를 하강 궤적으로 이동시키고 카메라가 추적.
// 지면 충돌 시 폭발 파티클 + 사운드, 카메라 복구 후 종료. (런타임 AddEvent — Initialize 미사용)
class HelicopterCrashEvent : public IOneShotEvent {
public:
	// bLandMode=true 면 추락/폭발 대신 착륙(구조 헬기). 착륙 후 헬기는 월드에 남고,
	// IsLanded()/GetExtractionPos() 로 탈출 시퀀스(GameScene)가 이어받는다.
	HelicopterCrashEvent(float fDuration, bool bLandMode = false)
		: m_fDuration{ fDuration > 0.f ? fDuration : 10.0f }, m_bLandMode{ bLandMode } {}
	virtual void OnEnterEvent(Scene* pScene) override;
	virtual void OnUpdateEvent(Scene* pScene) override;

	// 착륙 모드 결과 (탈출 시퀀스 연동용)
	bool IsLanded() const { return m_bLanded; }
	const Vector3& GetExtractionPos() const { return m_v3ExtractionPos; }
	// 착륙한 헬기 오브젝트 (출발 컷씬에서 그대로 띄워 올리기 위해). 착륙 모드는 Finish에서 reset 안 함.
	std::shared_ptr<HelicopterObject> GetHeli() const { return m_pHeli; }

private:
	void SpawnExplosion(const Vector3& v3Pos);
	void Finish(Scene* pScene);

private:
	float m_fDuration;
	float m_fTimeElapsed = 0.f;
	bool  m_bExploded = false;
	bool  m_bCinematicPushed = false; // 게임플레이 정지 중복 Push/Pop 방지

	bool    m_bLandMode = false;       // true=구조 헬기 착륙(폭발 X)
	bool    m_bLanded = false;         // 착륙 완료 플래그
	Vector3 m_v3ExtractionPos{};       // 착륙 지점 = 탈출 지점

	// OnEnter서 플레이어 기준으로 산출 (정면 하늘 → 정면 지면)
	Vector3 m_v3HeliStart{};
	Vector3 m_v3HeliEnd{};
	Vector3 m_v3LookAt{};      // 부드러운 추적용 시선 타겟 (헬기로 지연 수렴)
	Vector3 m_v3PlayerPos{};   // OnEnter서 고정한 플레이어 위치 (카메라가 매 프레임 이 뒤에 배치)

	// 언리얼에서 내보낸 헬기 비행 경로점(cm). 2개 이상이면 이 경로를 보간해 비행,
	// 비어 있으면 위 m_v3HeliStart→End 직선 폴백.
	std::vector<Vector3> m_v3PathPoints;

	std::shared_ptr<Camera>      m_pCinematicCamera;
	std::shared_ptr<Camera>      m_pSavedCamera;
	std::shared_ptr<HelicopterObject> m_pHeli; // Gunship 헬기(모델+로터). 착륙 모드는 Finish에서 유지.
};

// 탈출 출발 컷씬 (마지막 F키 탈출 시): 전 플레이어 숨김 + 착륙 헬기를 ArrivePath 역방향으로
// 상승시켜 경로 끝(상공)에 도달하면 종료. 카메라는 하늘 고정 위치(추적 X).
class HelicopterDepartEvent : public IOneShotEvent {
public:
	// pHeli: 착륙한 헬기(그대로 재사용). reversedPath: ArrivePath 역순(착륙점→상공).
	HelicopterDepartEvent(std::shared_ptr<HelicopterObject> pHeli, std::vector<Vector3> reversedPath, float fDuration)
		: m_pHeli{ std::move(pHeli) }, m_v3PathPoints{ std::move(reversedPath) }
		, m_fDuration{ fDuration > 0.f ? fDuration : 8.0f } {}
	virtual void OnEnterEvent(Scene* pScene) override;
	virtual void OnUpdateEvent(Scene* pScene) override;

private:
	void Finish(Scene* pScene);

private:
	std::shared_ptr<HelicopterObject> m_pHeli;
	std::vector<Vector3>         m_v3PathPoints;   // [0]=착륙점 … [last]=상공
	float                        m_fDuration;
	float                        m_fTimeElapsed = 0.f;
	bool                         m_bCinematicPushed = false;

	std::shared_ptr<Camera>      m_pCinematicCamera;
	std::shared_ptr<Camera>      m_pSavedCamera;
};

class CinematicCameraEvent : public ITriggerEvent {
public:
	virtual void Initialize(Scene* pScene) override;
	virtual void OnEnterEvent(Scene* pScene) override;
	virtual void OnUpdateEvent(Scene* pScene) override;
	virtual void OnLeaveEvent(Scene* pScene) override;

private:
	float m_fTotalPlayTime = 30.f;
	float m_fTimeElapsed = 0.f;

	bool m_bPlayed = false;
	std::vector<Vector3> m_v3ControlPoints;
	std::shared_ptr<Camera> m_pCinematicCamera;
	std::shared_ptr<Camera> m_pCameraSwapped;
};

// 1. 
