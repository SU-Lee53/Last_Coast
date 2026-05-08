#pragma once
#include "Zombie.h"

// 좀비 메모리 풀.
// 모든 Zombie 객체를 미리 할당하고 영구 보관 — 게임플레이 중 new/delete 없음.
//
// [폴링 → 이벤트 방식]
// 좀비가 죽는 순간 MarkForRelease()로 직접 알림 → m_PendingRelease에 적재.
// Collect()는 m_PendingRelease만 처리 — O(이번 프레임 사망 수), 전체 순회 없음.
// m_nActiveIndex로 m_Active 내 위치를 O(1)에 찾아 swap-with-last 제거.
class ZombiePool {
public:
    // capacity만큼 Zombie를 미리 생성 후 반환.
    // 반환된 shared_ptr들을 호출자가 m_pGameObjects에 추가해야 함.
    // bStartDormant=false(기본): 모든 좀비가 즉시 active 상태로 시작.
    // bStartDormant=true       : 모든 좀비가 m_Free에 대기 (서버 스폰 대기).
    std::vector<std::shared_ptr<Zombie>> Initialize(int capacity, bool bStartDormant = false);

    // m_Free에서 좀비 하나를 꺼내 활성화. 풀 고갈 시 nullptr.
    std::shared_ptr<Zombie> Acquire();

    // 좀비가 죽는 순간 직접 호출 — m_PendingRelease에 등록.
    // Zombie::PostUpdate() 내부에서 호출됨. 외부에서 직접 호출 금지.
    void MarkForRelease(std::shared_ptr<Zombie> pZombie);

    // m_PendingRelease 목록만 처리 → PoolReset() + swap-with-last.
    // 전체 순회 없이 O(이번 프레임 사망 수)로 동작.
    void Collect();

    int GetActiveCount() const { return static_cast<int>(m_Active.size()); }
    int GetFreeCount()   const { return static_cast<int>(m_Free.size()); }

    const std::vector<std::shared_ptr<Zombie>>& GetActive() const { return m_Active; }

private:
    std::vector<std::shared_ptr<Zombie>> m_All;           // 전체 좀비 — 영구 소유
    std::vector<std::shared_ptr<Zombie>> m_Active;        // 현재 살아있는 좀비
    std::vector<std::shared_ptr<Zombie>> m_Free;          // 재사용 대기 중인 좀비
    std::vector<std::shared_ptr<Zombie>> m_PendingRelease; // 이번 프레임 사망 알림 목록
};
