#pragma once

namespace AIDLL
{
    struct SensoryRecord
    {
        int     nEntityId         = -1;
        Vector3 v3LastKnownPos    = {};
        float   fTimeSinceSeen    = 15.0f;  // seconds since last perceived
        bool    bCurrentlyVisible = false;
        bool    bCurrentlyHeard   = false;
        bool    bInvestigated     = false;  // 마지막 위치 조사 완료 여부 (새 접촉 시 리셋)
    };

    class SensoryMemory
    {
    public:
        static constexpr float g_fMemorySpan = 15.0f;  // 15 seconds

        // Call AFTER UpdateStimulus each frame — increments decay timer for un-perceived entities
        void Update(float deltaTime);

        // Call each frame with current perception state (before Think/decision)
        void UpdateStimulus(int id, const Vector3& pos, bool visible, bool heard);

        bool CanSee(int id)      const;
        bool CanHear(int id)     const;
        bool HasMemoryOf(int id) const;  // fTimeSinceSeen < g_fMemorySpan
        bool IsInvestigated(int id) const;
        void SetInvestigated(int id);
        const SensoryRecord* GetRecord(int id) const;

    private:
        std::unordered_map<int, SensoryRecord> m_Records;
    };
}
