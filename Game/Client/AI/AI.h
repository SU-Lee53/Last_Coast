#pragma once

#ifdef AIDLL_EXPORTS
#define AI_API __declspec(dllexport)
#else
#define AI_API __declspec(dllimport)
#endif



namespace AIDLL
{
	// ✅ SimpleMath의 Vector3 사용
	using Vector3 = DirectX::SimpleMath::Vector3;

	// ========================================
	// NavMesh
	// ========================================
	struct NavMeshConfig
	{
		float CellSize;
		float CellHeight;
		float AgentRadius;
		float AgentHeight;
		float AgentMaxSlope;
		float AgentMaxStepHeight;
	};

	interface AI_API INavMesh
	{
	public:
		virtual ~INavMesh() {}

		virtual bool LoadFromJson(const char* filepath) = 0;
		virtual bool IsPointOnNavMesh(const Vector3& point) const = 0;
		virtual Vector3 GetNearestPointOnNavMesh(const Vector3& point) const = 0;
		virtual bool FindPath(const Vector3& start, const Vector3& end, std::vector<Vector3>& outPath) = 0;
		virtual NavMeshConfig GetConfig() const = 0;
		virtual int GetTileCount() const = 0;
	};

	// ========================================
	// AI Agent
	// ========================================
	enum class AIState
	{
		Idle,
		Moving,
		Attacking,
		Fleeing
	};

	interface AI_API IAIAgent
	{
	public:
		virtual ~IAIAgent() {}

		virtual void SetNavMesh(INavMesh* navMesh) = 0;
		virtual void Update(float deltaTime) = 0;
		virtual void MoveToPosition(const Vector3& target) = 0;
		virtual void SetPosition(const Vector3& position) = 0;
		virtual Vector3 GetPosition() const = 0;
		virtual AIState GetState() const = 0;
	};

	// ========================================
	// AI Manager
	// ========================================
	interface AI_API IAIManager
	{
	public:
		virtual ~IAIManager() {}

		// NavMesh
		virtual INavMesh* GetNavMesh() = 0;
		virtual bool LoadNavMesh(const char* filepath) = 0;

		// Agent 관리
		virtual IAIAgent* CreateAgent() = 0;
		virtual void DestroyAgent(IAIAgent* agent) = 0;
		virtual void UpdateAll(float deltaTime) = 0;
	};

	// ========================================
	// Factory 함수
	// ========================================
	extern "C" AI_API IAIManager* CreateAIManager();
	extern "C" AI_API void DestroyAIManager(IAIManager* manager);
}
