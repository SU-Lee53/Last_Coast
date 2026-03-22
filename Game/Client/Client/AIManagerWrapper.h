#pragma once

class AIManagerWrapper
{
	DECLARE_SINGLE(AIManagerWrapper)

private:
	std::shared_ptr<IAIManager> m_pAIManager;  // CreateAIManager가 shared_ptr 반환
	bool m_bInitialized;

public:
	// 초기화/종료
	bool Initialize(const std::string& strFileName);
	void Shutdown();
	bool IsInitialized() const { return m_bInitialized; }

	// 에이전트 관리
	std::shared_ptr<IAIAgent> CreateAgent();
	// 업데이트
	void UpdateAll(float deltaTime);

	// NavMesh 쿼리
	bool IsPointOnNavMesh(const Vector3& point);
	Vector3 GetNearestPointOnNavMesh(const Vector3& point);

	std::shared_ptr<INavMesh> GetNavMesh() { return m_pAIManager->GetNavMesh(); }

	// 경보 전파
	void SpreadAlert(const Vector3& SourcePos, int EntityId,
	                 const Vector3& TargetPos, float Radius);
};
