#include "pch.h"
#include "SensoryMemory.h"

namespace AIDLL
{
    // ─────────────────────────────────────────────────────────────────────────
    // Update : 감지되지 않는 엔티티의 경과 시간 누적 (Decay)
    // UpdateStimulus 호출 이후에 호출할 것
    // ─────────────────────────────────────────────────────────────────────────
    void SensoryMemory::Update(float deltaTime)
    {
        for (auto& [id, rec] : m_Records)
        {
            if (!rec.bCurrentlyVisible && !rec.bCurrentlyHeard)
                rec.fTimeSinceSeen += deltaTime;
        }
    }

    // ─────────────────────────────────────────────────────────────────────────
    // UpdateStimulus : 현재 프레임의 지각 상태를 기록
    // ─────────────────────────────────────────────────────────────────────────
    void SensoryMemory::UpdateStimulus(int id, const Vector3& pos, bool visible, bool heard)
    {
        auto& rec = m_Records[id];
        rec.nEntityId = id;
        rec.bCurrentlyVisible = visible;
        rec.bCurrentlyHeard = heard;

        if (visible || heard)
        {
            rec.v3LastKnownPos = pos;
            rec.fTimeSinceSeen = 0.0f;
            rec.bInvestigated  = false;  // 새 접촉 → 조사 완료 플래그 리셋
        }
    }

    bool SensoryMemory::CanSee(int id) const
    {
        auto it = m_Records.find(id);
        if (it == m_Records.end()) 
			return false;
        return it->second.bCurrentlyVisible;
    }

    bool SensoryMemory::CanHear(int id) const
    {
        auto it = m_Records.find(id);
        if (it == m_Records.end()) 
			return false;
        return it->second.bCurrentlyHeard;
    }

    bool SensoryMemory::IsInvestigated(int id) const
    {
        auto it = m_Records.find(id);
        if (it == m_Records.end()) return false;
        return it->second.bInvestigated;
    }

    void SensoryMemory::SetInvestigated(int id)
    {
        auto it = m_Records.find(id);
        if (it != m_Records.end())
            it->second.bInvestigated = true;
    }

    bool SensoryMemory::HasMemoryOf(int id) const
    {
        auto it = m_Records.find(id);
        if (it == m_Records.end()) 
			return false;
        return it->second.fTimeSinceSeen < g_fMemorySpan;
    }

    const SensoryRecord* SensoryMemory::GetRecord(int id) const
    {
        auto it = m_Records.find(id);
        if (it == m_Records.end()) 
			return nullptr;
        return &it->second;
    }
}
