#pragma once
#include "AI.h"

namespace AIDLL
{
	class AIAgentImpl : public IAIAgent
	{
	public:
		AIAgentImpl();
		virtual ~AIAgentImpl();

		virtual void SetNavMesh(INavMesh* navMesh) override;
		virtual void Update(float deltaTime) override;
		virtual void MoveToPosition(const Vector3& target) override;
		virtual Vector3 GetPosition() const override { return m_Position; }
		virtual AIState GetState() const override { return m_State; }

	private:
		INavMesh* m_pNavMesh;
		Vector3 m_Position;
		AIState m_State;
		std::vector<Vector3> m_Path;
		int m_PathIndex;
		float m_MoveSpeed;
	};
}

