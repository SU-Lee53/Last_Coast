#pragma once
#include "pch.h"

namespace AIDLL
{
	class AIPathPlanner;  // 전방 선언만

	class PathManager
	{
	private:
		std::list<std::weak_ptr<AIPathPlanner>> m_SearchRequests;
		int m_nNumSearchCyclesPerUpdate;
	public:
		PathManager(unsigned int NumCyclesPerUpdate) : m_nNumSearchCyclesPerUpdate(NumCyclesPerUpdate) {}
		void UpdateSearches();
		void Register(std::shared_ptr<AIPathPlanner> pPathPlanner);
		void UnRegister(AIPathPlanner* pPathPlanner);
		int  GetNumActiveSearches() const { return (int)m_SearchRequests.size(); }
	};

} // namespace AIDLL
