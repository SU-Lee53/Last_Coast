#include "pch.h"
#include "CheckpointSystem.h"
#include "GameWorld.h"
#include "Session.h"

#define JSON_HAS_RANGES 0
#include <Includes/nlohmann_json/json.hpp>

int CheckpointSystem::Load(const std::string& path)
{
	m_Checkpoints.clear();

	std::ifstream in(path);
	if (!in) {
		std::cout << "[Checkpoint] file not found: " << path << "\n";
		return 0;
	}

	nlohmann::json j = nlohmann::json::parse(in, nullptr, false);
	if (j.is_discarded() || !j.contains("Checkpoints"))
		return 0;

	for (const auto& jc : j["Checkpoints"]) {
		const auto& wm = jc["Transform"]["WorldMatrix"]; // 행 우선 4x4 — translation = 12,13,14
		Vector3 v3Pos(wm[12].get<float>(), wm[13].get<float>(), wm[14].get<float>());
		m_Checkpoints.push_back(Checkpoint{ v3Pos, {}, false });
	}

	std::cout << "[Checkpoint] loaded: " << m_Checkpoints.size() << "\n";
	return static_cast<int>(m_Checkpoints.size());
}

void CheckpointSystem::AssignEvents()
{
	auto Set = [this](size_t i, std::vector<EventEmit> ev) {
		if (i < m_Checkpoints.size()) m_Checkpoints[i].events = std::move(ev);
		};

	// 예시 배정 — 한 체크포인트가 여러 이벤트를 동시에 쏠 수 있음.
	//Set(0, { { GE_HELICOPTER_CRASH, {}, 0.f, 0.0f, 0, 5.0f } });
	Set(0, { { GE_ENVIRONMENT, {}, 0.f, 8.0f, EP_DAWN, 0.0f } });   // 0번 도착 → 석양 (정지 8초)
	Set(1, { { GE_HELICOPTER_CRASH, {}, 0.f, 10.f, 0, 10.0f } ,{ GE_ENVIRONMENT, {}, 0.f, 8.0f, EP_SUNSET, 0.0f } });
	Set(2, { { GE_HELICOPTER_ARRIVE, {}, 0.f, 10.f, 0, 10.0f } });
}

void CheckpointSystem::Update(float minPlayerX, DWORD now, GameWorld& world)
{
	for (auto& cp : m_Checkpoints)
	{
		if (cp.bFired || minPlayerX < cp.v3Pos.x) continue;
		cp.bFired = true;

		float fMaxFreeze = 0.f;
		for (const auto& e : cp.events) {
			BroadcastAll([&](Session& cl) {
				cl.send_game_event(e.eventId, e.pos, e.fTargetValue, e.fDuration, e.presetId);
				});
			fMaxFreeze = std::max(fMaxFreeze, e.fFreezeDuration);

			// 구조 헬기 착륙 이벤트 → 탈출 시퀀스 시작. 서바이벌은 컷씬(정지) 종료 후부터.
			if (e.eventId == GE_HELICOPTER_ARRIVE)
				world.GetEscape().BeginSurvival(now, e.fFreezeDuration, world.GetZombies());
		}

		// 컷씬이면 그 길이(+0.5초 버퍼)만큼 서버 좀비 정지 — 클라 컷씬 종료 시점까지 좀비 고정.
		if (fMaxFreeze > 0.f)
			world.ExtendZombieFreeze(now + static_cast<DWORD>((fMaxFreeze + 0.5f) * 1000.f));

		std::cout << "[Checkpoint] fired at x=" << cp.v3Pos.x
			<< " (" << cp.events.size() << " events, freeze " << fMaxFreeze << "s)\n";
	}
}
