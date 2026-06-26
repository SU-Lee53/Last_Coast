#include "pch.h"
#include "GameEvent.h"
#include "Skybox.h"
#include "BloodEffect.h"
#include "MuzzleFlashEffect.h"
#include "ToneMappingVolume.h"
#include "NodeObject.h"
#include "CrashDebris.h"
#include "SpatialTraits.h"

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
	// TODO: 전용 ExplosionEffect 제작 시 교체. 지금은 MuzzleFlash 임시 재활용.
	ParticleEffectSpawnDesc desc;
	desc.v3Position  = m_v3Pos;
	desc.v3Direction = Vector3::Up;
	desc.v3Normal    = Vector3::Up;
	desc.mtxWorld    = Matrix::CreateWorld(desc.v3Position, desc.v3Direction, desc.v3Normal);
	PARTICLE->Spawn<MuzzleFlashEffect>(desc);

	SOUND->PlayAt("Test3D", m_v3Pos);

	m_bFinished = true; // 1회 재생 후 즉시 종료
}

void PostFXFadeEvent::OnEnterEvent(Scene* pScene)
{
	m_fStartScale  = pScene->GetToneMappingVolume().GetOutputScale();
	m_fTimeElapsed = 0.f;
}

void PostFXFadeEvent::OnUpdateEvent(Scene* pScene)
{
	m_fTimeElapsed += DT;
	float t = (m_fDuration > 0.f) ? std::clamp(m_fTimeElapsed / m_fDuration, 0.f, 1.f) : 1.f;

	float fScale = m_fStartScale + (m_fTargetScale - m_fStartScale) * t;
	pScene->GetToneMappingVolume().SetOutputScale(fScale);

	if (t >= 1.f) m_bFinished = true;
}

void HorrorLookEvent::OnEnterEvent(Scene* pScene)
{
	auto& tone = pScene->GetToneMappingVolume().GetCommonParameters();
	auto& fx   = pScene->GetPostProcessingVolume().GetScreenFXParameters();

	m_fStartExposure = tone.fExposure;
	m_fStartOutput   = tone.fOutputScale;
	m_fStartSat      = tone.fPostSaturation;
	m_fStartVignette = fx.fVignetteStrength;

	tone.nEnableAutoExposure = 0; // exposure 수동 제어 (auto면 무시됨)
	m_fTimeElapsed = 0.f;
}

void HorrorLookEvent::OnUpdateEvent(Scene* pScene)
{
	m_fTimeElapsed += DT;
	float t = (m_fDuration > 0.f) ? std::clamp(m_fTimeElapsed / m_fDuration, 0.f, 1.f) : 1.f;

	auto& tone = pScene->GetToneMappingVolume().GetCommonParameters();
	auto& fx   = pScene->GetPostProcessingVolume().GetScreenFXParameters();

	tone.fExposure       = std::lerp(m_fStartExposure, 0.35f, t);
	tone.fOutputScale    = std::lerp(m_fStartOutput,   0.55f, t);
	tone.fPostSaturation = std::lerp(m_fStartSat,      0.40f, t);
	fx.fVignetteStrength = std::lerp(m_fStartVignette, 0.70f, t);

	if (t >= 1.f) m_bFinished = true;
}

void RestoreLookEvent::OnEnterEvent(Scene* pScene)
{
	auto& tone = pScene->GetToneMappingVolume().GetCommonParameters();
	auto& fx   = pScene->GetPostProcessingVolume().GetScreenFXParameters();

	m_fStartExposure = tone.fExposure;
	m_fStartOutput   = tone.fOutputScale;
	m_fStartSat      = tone.fPostSaturation;
	m_fStartVignette = fx.fVignetteStrength;
	m_fTimeElapsed   = 0.f;
}

void RestoreLookEvent::OnUpdateEvent(Scene* pScene)
{
	m_fTimeElapsed += DT;
	float t = (m_fDuration > 0.f) ? std::clamp(m_fTimeElapsed / m_fDuration, 0.f, 1.f) : 1.f;

	auto& tone = pScene->GetToneMappingVolume().GetCommonParameters();
	auto& fx   = pScene->GetPostProcessingVolume().GetScreenFXParameters();

	// 기본값으로 복귀 (ToneMappingCommonParameters / ScreenFXParameters 디폴트)
	tone.fExposure       = std::lerp(m_fStartExposure, 1.0f,  t);
	tone.fOutputScale    = std::lerp(m_fStartOutput,   1.0f,  t);
	tone.fPostSaturation = std::lerp(m_fStartSat,      1.0f,  t);
	fx.fVignetteStrength = std::lerp(m_fStartVignette, 0.25f, t);

	if (t >= 1.f) {
		tone.nEnableAutoExposure = 1; // auto-exposure 복원
		m_bFinished = true;
	}
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
	const char*   HELI_MODEL_NAME = "SM_Cube";          // 헬기 모델 이름(현재 placeholder)
	const Vector3 HELI_SCALE{ 1.f, 1.f, 1.f };         // placeholder 큐브용 스케일. 실제 모델은 보통 {1,1,1}

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

	// 시네마틱 카메라 구성 후 현재 카메라와 교체
	m_pCinematicCamera = std::make_shared<Camera>();
	m_pCinematicCamera->SetViewport(0, 0, WinCore::g_dwClientWidth, WinCore::g_dwClientHeight, 0.f, 1.f);
	m_pCinematicCamera->SetScissorRect(0, 0, WinCore::g_dwClientWidth, WinCore::g_dwClientHeight);
	m_pCinematicCamera->GenerateViewMatrix(XMFLOAT3(0.f, 0.f, 0.f), XMFLOAT3(0.f, 0.f, 1.f), XMFLOAT3(0.f, 1.f, 0.f));
	m_pCinematicCamera->GenerateProjectionMatrix(
		10.f, 1000_m, 
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
	m_v3PathPoints = pScene->GetHeliPath();
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

	// 헬기 오브젝트 — 월드/스폐셜 미등록, 컷씬 prop으로 직접 렌더.
	// 모델 교체는 상단 HELI_MODEL_NAME / HELI_SCALE 만 수정.
	m_pHeli = MODEL->LoadOrGet(HELI_MODEL_NAME, true)->CopyObject<CrashDebris>();
	m_pHeli->Initialize();
	m_pHeli->GetTransform()->SetFrameMatrix(Matrix::Identity); // 루트 frame 분리 → world = transform
	pScene->SetCinematicProp(m_pHeli);
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

	// 기수를 진행 방향으로 정렬 (진행 벡터가 너무 작으면 회전 생략)
	Matrix mtxRot = Matrix::Identity;
	if (v3Travel.LengthSquared() > 1e-4f) {
		v3Travel.Normalize();
		Vector3 v3Up = Vector3::Up;
		if (fabsf(v3Travel.Dot(v3Up)) > 0.99f) v3Up = Vector3::Backward; // 수직 비행 시 up 특이점 회피
		mtxRot = Matrix::CreateWorld(Vector3::Zero, v3Travel, v3Up); // forward 정렬(translation 0)
	}

	const Matrix mtxWorld =
		Matrix::CreateScale(HELI_SCALE) *
		mtxRot *
		Matrix::CreateTranslation(v3HeliPos);
	m_pHeli->GetTransform()->SetWorldMatrix(mtxWorld);
	m_pHeli->PostUpdate(); // 루트 → 자식 메시 world 행렬 전파

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

	// 지면 충돌 → 폭발 + 충돌점에 헬기 잔해로 남기고 종료
	if (!m_bExploded && t >= 1.f) {
		SpawnExplosion(m_v3HeliEnd);
		m_bExploded = true;
		// 헬기를 충돌점 잔해로 월드에 등록 → 좀비와 동일한 정상 렌더/그림자 파이프라인.
		// (이번 프레임 위치 갱신으로 m_pHeli world는 이미 충돌점 자세. prop 해제는 Finish가 처리)
		if (pScene && m_pHeli) pScene->AddObject(m_pHeli);
		Finish(pScene);
	}
}

void HelicopterCrashEvent::SpawnExplosion(const Vector3& v3Pos)
{
	// TODO: 전용 ExplosionEffect 제작 시 교체 (지금은 MuzzleFlash 임시 재활용)
	ParticleEffectSpawnDesc desc;
	desc.v3Position  = v3Pos;
	desc.v3Direction = Vector3::Up;
	desc.v3Normal    = Vector3::Up;
	desc.mtxWorld    = Matrix::CreateWorld(desc.v3Position, desc.v3Direction, desc.v3Normal);
	PARTICLE->Spawn<MuzzleFlashEffect>(desc);

	SOUND->PlayAt("Test3D", v3Pos);
}

void HelicopterCrashEvent::Finish(Scene* pScene)
{
	if (pScene) {
		if (m_pSavedCamera) {
			pScene->SwapCamera(m_pSavedCamera);
		}
		pScene->ClearCinematicProp();
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
