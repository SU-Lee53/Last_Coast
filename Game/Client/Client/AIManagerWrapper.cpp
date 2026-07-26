#include "pch.h"
#include "AIManagerWrapper.h"

bool AIManagerWrapper::Initialize(const std::string& strFileName)
{
	if (m_bInitialized)
		return true;

	// DLL에서 AIManager 생성
	m_pAIManager = CreateAIManager();  // shared_ptr 직접 할당
	if (!m_pAIManager)
		return false;

	// NavMesh 로드
	if (!m_pAIManager->LoadNavMesh(strFileName))
	{
		m_pAIManager.reset();  // shared_ptr 자동 삭제
		return false;
	}

	m_bInitialized = true;
	return true;
}

void AIManagerWrapper::Shutdown()
{
	if (m_pAIManager)
	{
		m_pAIManager.reset();  // shared_ptr 자동 삭제
		m_bInitialized = false;
	}
}
std::shared_ptr<IAIAgent> AIManagerWrapper::CreateAgent()
{
	if (!m_pAIManager)
	{
		assert(false && "AIManager not initialized!");
		return nullptr;
	}

	return m_pAIManager->CreateAgent();
}

void AIManagerWrapper::UpdateAll(float deltaTime)
{
	if (m_pAIManager)
		m_pAIManager->UpdateAll(deltaTime);
}

bool AIManagerWrapper::IsPointOnNavMesh(const Vector3& point)
{
	if (!m_pAIManager)
		return false;

	return m_pAIManager->GetNavMesh()->IsPointOnNavMesh(point);
}

Vector3 AIManagerWrapper::GetNearestPointOnNavMesh(const Vector3& point)
{
	if (!m_pAIManager)
		return point;

	return m_pAIManager->GetNavMesh()->GetNearestPointOnNavMesh(point);
}

void AIManagerWrapper::SpreadAlert(const Vector3& SourcePos, int EntityId,
                                   const Vector3& TargetPos, float Radius)
{
	if (m_pAIManager)
		m_pAIManager->SpreadAlert(SourcePos, EntityId, TargetPos, Radius);
}

void AIManagerWrapper::SpreadDistraction(const Vector3& SourcePos, float Radius, float Duration)
{
	if (m_pAIManager)
		m_pAIManager->SpreadDistraction(SourcePos, Radius, Duration);
}
