#pragma once

#ifdef AI_EXPORTS
#define AI_API __declspec(dllexport)
#else
#define AI_API __declspec(dllimport)
#endif

namespace AIDLL
{
	// ========================================
	// NavMesh
	// ========================================
	struct NavMeshConfig
	{
		float fCellSize;
		float fCellHeight;
		float fAgentRadius;
		float fAgentHeight;
		float fAgentMaxSlope;
		float fAgentMaxStepHeight;
	};

	// ========================================
	// AI Agent
	// ========================================
	enum class AIPathState
	{
		Idle,
		PathRequested,  // 비동기 경로탐색 대기 중
		Moving,
	};

	// Goal 기반 행동 상태 (GoalZombieThink 출력)
	enum class AIBehaviorState
	{
		Idle = 0,  
		Wandering = 1,
		Alert = 2, 
		Investigating = 3, 
		Chasing = 4, 
		Attacking = 5
	};

	// 디버그용 경로 정보
	struct AI_API PathDebugInfo
	{
		struct Portal
		{
			Vector3 v3Left;
			Vector3 v3Right;
			int nPolyA;
			int nPolyB;
		};

		std::vector<Vector3> Waypoints;
		std::vector<Portal> Portals;
		std::vector<Vector3> PolyNodeCenters;  // A* 가 찾은 폴리곤 시퀀스 무게중심 (경로 노드 시각화용)
		Vector3 v3StartPos;
		Vector3 v3EndPos;
	};

	// 디버그용 NavMesh 렌더링 데이터
	struct AI_API NavMeshDebugData
	{
		std::vector<Vector3> PolygonEdges;    // 폴리곤 외곽선 (녹색으로 렌더링)
		std::vector<Vector3> AdjacencyEdges;  // 인접성 그래프 - 폴리곤 중심점 연결 (빨간색으로 렌더링)
	};

	interface AI_API INavMesh
	{
	public:
		virtual ~INavMesh() {}

		virtual bool LoadFromJson(const std::string & strFileName) = 0;
		virtual bool IsPointOnNavMesh(const Vector3& point) const = 0;
		virtual Vector3 GetNearestPointOnNavMesh(const Vector3& point) const = 0;
		virtual Vector3 GetRandomPoint() const = 0;  // 랜덤 NavMesh 위치 반환 (좀비 스폰 등)

		// 두 점 사이의 시야선이 NavMesh 경계(벽)에 막히지 않으면 true 반환
		virtual bool IsLineOfSightClear(const Vector3& From, const Vector3& To) const = 0;
		//virtual bool FindPath(const Vector3& start, const Vector3& end, std::vector<Vector3>& outPath) = 0;
		virtual NavMeshConfig GetConfig() const = 0;
		virtual int GetTileCount() const = 0;

		// 디버그 렌더링용 - 모든 폴리곤 엣지를 라인으로 반환
		virtual NavMeshDebugData GetDebugLineVertices() const = 0;
	};


	interface AI_API IAIAgent
	{
	public:
		virtual ~IAIAgent() {}

		// SetNavMesh는 내부 구현 전용이므로 인터페이스에서 제거
		virtual void Update(float deltaTime) = 0;
		virtual void MoveToPosition(const Vector3& target) = 0;
		virtual void SetPosition(const Vector3& position) = 0;
		virtual void SyncPosition(const Vector3& position) = 0;		// 경로/상태 초기화 없이 위치만 갱신 (충돌 해소 후 실제 위치 동기화용)
		virtual void SetMoveSpeed(float fSpeed) = 0;
		virtual Vector3 GetPosition() const = 0;
		virtual AIPathState GetPathState() const = 0;
		virtual const PathDebugInfo& GetPathDebugInfo() const = 0;

		// Goal 기반 행동 제어
		virtual AIBehaviorState GetBehaviorState() const = 0;
		virtual void UpdateSensoryStimulus(int EntityId, const Vector3& pos,
		                                   bool Visible, bool Heard) = 0;
		virtual void Think(int nTargetEntityId, float deltaTime, float fTargetDist = FLT_MAX) = 0;

		// 공격 히트 이벤트 소비 (GoalAttack 쿨다운 완료 시 true, 한 번만 반환)
		virtual bool ConsumeAttackHit() = 0;
	};

	// ========================================
	// AI Manager
	// ========================================
	interface AI_API IAIManager
	{
	public:
		virtual ~IAIManager() {}

		// NavMesh
		virtual std::shared_ptr<INavMesh> GetNavMesh() = 0;
		virtual bool LoadNavMesh(const std::string& strFileName) = 0;

		// Agent 관리
		virtual std::shared_ptr<IAIAgent> CreateAgent() = 0;
		virtual void UpdateAll(float deltaTime) = 0;

		// 경보 전파: v3SourcePos 기준 fRadius 안에 있는 에이전트들에게
		//           nEntityId 의 위치(v3TargetPos)를 청각 자극으로 전달
		virtual void SpreadAlert(const Vector3& SourcePos, int EntityId,
		                         const Vector3& TargetPos, float Radius) = 0;
	};

	// ========================================
	// Factory 함수
	// ========================================
	AI_API std::shared_ptr<IAIManager> CreateAIManager();
}
