#pragma once
#include "AI.h"
#include "NavMeshImpl.h"
#include "AIAgentImpl.h"

namespace AIDLL
{
	class AIManagerImpl : public IAIManager
	{
	public:
		AIManagerImpl();
		virtual ~AIManagerImpl();

		// NavMesh
		virtual INavMesh* GetNavMesh() override { return m_pNavMesh.get(); }
		virtual bool LoadNavMesh(const char* filepath) override;

		// Agent 관리
		virtual IAIAgent* CreateAgent() override;
		virtual void DestroyAgent(IAIAgent* agent) override;
		virtual void UpdateAll(float deltaTime) override;

	private:
		std::unique_ptr<NavMeshImpl> m_pNavMesh;
		std::vector<std::unique_ptr<AIAgentImpl>> m_Agents;
	};
}

