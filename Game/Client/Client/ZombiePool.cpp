#include "pch.h"
#include "ZombiePool.h"

std::vector<std::shared_ptr<Zombie>> ZombiePool::Initialize(int capacity)
{
    m_All.reserve(capacity);
    m_Active.reserve(capacity);

    for (int i = 0; i < capacity; ++i)
    {
        auto pZombie = std::make_shared<Zombie>();
        pZombie->m_pPool        = this;
        pZombie->m_nActiveIndex = i;
        m_All.push_back(pZombie);
        m_Active.push_back(pZombie);
    }

    return m_All;
}

std::shared_ptr<Zombie> ZombiePool::Acquire()
{
    if (m_Free.empty()) return nullptr;

    std::shared_ptr<Zombie> pZombie = m_Free.back();
    m_Free.pop_back();

    pZombie->m_nActiveIndex = static_cast<int>(m_Active.size());
    m_Active.push_back(pZombie);

    pZombie->PoolActivate();
    return pZombie;
}

void ZombiePool::MarkForRelease(std::shared_ptr<Zombie> pZombie)
{
    m_PendingRelease.push_back(std::move(pZombie));
}

void ZombiePool::Collect()
{
    // 이번 프레임에 죽은 좀비만 처리 — 전체 순회 없음
    for (const auto& pZombie : m_PendingRelease)
    {
        int nIdx = pZombie->m_nActiveIndex;
        if (nIdx < 0 || nIdx >= static_cast<int>(m_Active.size())) continue;
        if (m_Active[nIdx] != pZombie) 
			continue; // 인덱스 불일치 방어

        m_Free.push_back(pZombie);
        pZombie->PoolReset(); // m_bPoolActive = false, m_nActiveIndex = -1

        // swap-with-last: 마지막 원소를 빈 자리로 이동하고 인덱스 갱신
        if (nIdx != static_cast<int>(m_Active.size()) - 1)
        {
            m_Active[nIdx] = m_Active.back();
            m_Active[nIdx]->m_nActiveIndex = nIdx;
        }
        m_Active.pop_back();
    }
    m_PendingRelease.clear();
}
