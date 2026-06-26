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

// 폭발 연출 (서버 GE_EXPLOSION). 지정 위치에 파티클 + 3D 사운드 1회 재생 후 종료.
class ExplosionEvent : public IOneShotEvent {
public:
	ExplosionEvent(const Vector3& v3Pos) : m_v3Pos{ v3Pos } {}
	virtual void OnEnterEvent(Scene* pScene) override;

private:
	Vector3 m_v3Pos;
};

// 화면 어둡게 페이드 (서버 GE_POSTFX_DARKEN). ToneMapping outputScale 를
// 현재값 → 목표값으로 지속시간 동안 보간. 완료 시 종료.
class PostFXFadeEvent : public IOneShotEvent {
public:
	PostFXFadeEvent(float fTargetScale, float fDuration)
		: m_fTargetScale{ fTargetScale }, m_fDuration{ fDuration } {}
	virtual void OnEnterEvent(Scene* pScene) override;
	virtual void OnUpdateEvent(Scene* pScene) override;

private:
	float m_fTargetScale;
	float m_fDuration;
	float m_fStartScale  = 1.f;
	float m_fTimeElapsed = 0.f;
};

// 작성형(A) 다중 파라미터 이벤트 예시: 호러 룩.
// exposure↓ + outputScale↓ + saturation↓ + vignette↑ 를 한 이벤트서 동시에 직접 조작.
// 새 "룩"이 필요하면 이런 식으로 이벤트 클래스 추가 — 패킷/enum에 값 넣을 필요 없음.
class HorrorLookEvent : public IOneShotEvent {
public:
	HorrorLookEvent(float fDuration) : m_fDuration{ fDuration } {}
	virtual void OnEnterEvent(Scene* pScene) override;
	virtual void OnUpdateEvent(Scene* pScene) override;

private:
	float m_fDuration;
	float m_fTimeElapsed   = 0.f;
	// 시작값 캡처
	float m_fStartExposure = 1.f;
	float m_fStartOutput   = 1.f;
	float m_fStartSat      = 1.f;
	float m_fStartVignette = 0.f;
};

// 포스트값을 기본으로 되돌리는 작성형 이벤트.
class RestoreLookEvent : public IOneShotEvent {
public:
	RestoreLookEvent(float fDuration) : m_fDuration{ fDuration } {}
	virtual void OnEnterEvent(Scene* pScene) override;
	virtual void OnUpdateEvent(Scene* pScene) override;

private:
	float m_fDuration;
	float m_fTimeElapsed   = 0.f;
	float m_fStartExposure = 1.f;
	float m_fStartOutput   = 1.f;
	float m_fStartSat      = 1.f;
	float m_fStartVignette = 0.f;
};

// 헬기 추락 컷씬 (서버 GE_HELICOPTER_CRASH). 시네마틱 카메라로 전환 후
// 임시 헬기 오브젝트(SM_Cube placeholder)를 하강 궤적으로 이동시키고 카메라가 추적.
// 지면 충돌 시 폭발 파티클 + 사운드, 카메라 복구 후 종료. (런타임 AddEvent — Initialize 미사용)
class HelicopterCrashEvent : public IOneShotEvent {
public:
	HelicopterCrashEvent(float fDuration)
		: m_fDuration{ fDuration > 0.f ? fDuration : 5.0f } {}
	virtual void OnEnterEvent(Scene* pScene) override;
	virtual void OnUpdateEvent(Scene* pScene) override;

private:
	void SpawnExplosion(const Vector3& v3Pos);
	void Finish(Scene* pScene);

private:
	float m_fDuration;
	float m_fTimeElapsed = 0.f;
	bool  m_bExploded = false;

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
	std::shared_ptr<CrashDebris> m_pHeli; // 충돌 후 월드 잔해로 등록(dynamic spatial)
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
