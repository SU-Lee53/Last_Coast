#include "pch.h"
#include "GameEvent.h"
#include "Skybox.h"
#include "BloodEffect.h"

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
