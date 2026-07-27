#include "pch.h"
#include "EscapeSequence.h"
#include "Session.h"
#include "ZombieManager.h"

namespace
{
	// 엔딩 서바이벌(탈출 대기) 동안 좀비 강화: 시야 무제한
	constexpr float ESCAPE_SIGHT_RANGE           = 1000000.f; // 사실상 무제한 감지 반경 (cm)
	constexpr float ESCAPE_SURVIVE_SECONDS       = 20.f;      // 버텨야 하는 시간
	constexpr DWORD ESCAPE_BROADCAST_INTERVAL_MS = 500;       // 상태 전송 주기
}

void EscapeSequence::BeginSurvival(DWORD now, float cutsceneSeconds, ZombieManager& zombies)
{
	m_ePhase = Phase::Survival;
	m_dwSurviveStart = now + static_cast<DWORD>((cutsceneSeconds + 0.5f) * 1000.f);
	m_dwLastBroadcast = 0;
	m_bRequested = false;
	zombies.SetSightRange(ESCAPE_SIGHT_RANGE); // 서바이벌 동안 시야 무제한
}

void EscapeSequence::Update(DWORD now, Room& room)
{
	// 착륙 컷씬 종료 후부터 카운트다운 → 상태를 주기적으로 방 인원에게 브로드캐스트.
	// 탈출 가능(Ready) 상태에서 누구든 탈출 키를 누르면 방 전원에게 게임 종료.
	if ((m_ePhase != Phase::Survival && m_ePhase != Phase::Ready) || now < m_dwSurviveStart)
		return;

	const float fElapsed = (now - m_dwSurviveStart) / 1000.f;
	float fRemain = ESCAPE_SURVIVE_SECONDS - fElapsed;
	if (fRemain <= 0.f) { fRemain = 0.f; m_ePhase = Phase::Ready; }

	const unsigned char ucPhase = (m_ePhase == Phase::Ready) ? 1 : 0;

	if (now - m_dwLastBroadcast >= ESCAPE_BROADCAST_INTERVAL_MS) {
		m_dwLastBroadcast = now;
		BroadcastRoom(&room, [&](Session& cl) { cl.send_escape_state(ucPhase, fRemain); });
	}

	if (m_ePhase == Phase::Ready && m_bRequested.exchange(false)) {
		m_ePhase = Phase::Ended;
		BroadcastRoom(&room, [&](Session& cl) { cl.send_game_end(); });
		std::cout << "[Escape] Room[" << room.room_id << "] player escaped -> GAME END broadcast\n";
	}
}
