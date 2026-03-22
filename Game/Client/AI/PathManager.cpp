#include "pch.h"
#include "PathManager.h"
#include "AIPathPlanner.h"      // 여기서 완전한 타입 include
#include "TimeSlicedGraphAlgorithms.h"

namespace AIDLL
{
	void PathManager::UpdateSearches()
	{
		int nNumCyclesRemaining = m_nNumSearchCyclesPerUpdate;
		auto CurPath = m_SearchRequests.begin();
		while (nNumCyclesRemaining-- && !m_SearchRequests.empty())
		{
			// weak_ptr이 expired되었는지 확인
			if (auto pathPlanner = CurPath->lock())
			{
				int nResult = pathPlanner->CycleOnce();
				if (nResult == target_found || nResult == target_not_found) {
					CurPath = m_SearchRequests.erase(CurPath);
				}
				else {
					++CurPath;
				}
			}
			else
			{
				// expired된 weak_ptr 제거
				CurPath = m_SearchRequests.erase(CurPath);
			}

			if (CurPath == m_SearchRequests.end()) {
				CurPath = m_SearchRequests.begin();
			}
		}
	}

	void PathManager::Register(std::shared_ptr<AIPathPlanner> pPathPlanner)
	{
		if (!pPathPlanner) return;

		// 이미 등록되어 있는지 확인
		for (const auto& weakPtr : m_SearchRequests)
		{
			if (auto existingPtr = weakPtr.lock())
			{
				if (existingPtr == pPathPlanner)
					return;  // 이미 등록됨
			}
		}

		m_SearchRequests.push_back(pPathPlanner);
	}

	void PathManager::UnRegister(AIPathPlanner* pPathPlanner)
	{
		if (!pPathPlanner) return;

		m_SearchRequests.remove_if([pPathPlanner](const std::weak_ptr<AIPathPlanner>& weakPtr)
		{
			if (auto sharedPtr = weakPtr.lock())
			{
				return sharedPtr.get() == pPathPlanner;
			}
			return true;  // expired된 것도 함께 제거
		});
	}

} // namespace AIDLL
