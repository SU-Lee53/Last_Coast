#pragma once
#include "pch.h"

class ZombieManager;
class Room;

// ─────────────────────────────────────────────────────────────────────────────
// 탈출 시퀀스 (마지막 체크포인트: 구조 헬기 착륙 후 서버가 주관) — 방별 인스턴스.
//   서버가 시간을 재서 상태(남은 시간)를 주기적으로 방 인원에게 보내고, 누구든
//   탈출 키를 누르면 방 전원에게 게임 종료를 브로드캐스트한다.
//   Update/BeginSurvival 은 게임 틱 스레드 전용, Request 만 워커 스레드에서 호출.
// ─────────────────────────────────────────────────────────────────────────────
class EscapeSequence {
public:
	enum class Phase { None, Survival, Ready, Ended };

	// 구조 헬기 착륙 이벤트(GE_HELICOPTER_ARRIVE) 발사 시 호출.
	// 서바이벌 카운트다운은 착륙 컷씬(정지) 종료 후부터 시작한다.
	void BeginSurvival(DWORD now, float cutsceneSeconds, ZombieManager& zombies);

	// 클라 탈출 키 입력 — 플래그만 세움. 실제 종료 판정/브로드캐스트는 틱 스레드가
	// 탈출 가능 상태(Ready)일 때만 처리한다 (스레드 안전 + 단일 권위).
	void Request() { m_bRequested = true; }

	// 카운트다운 진행 + 상태 브로드캐스트 + 탈출 판정 (room = 소속 방)
	void Update(DWORD now, Room& room);

	// 엔딩 서바이벌(탈출 대기) 중인가 — 좀비 스폰 강화 판단용
	bool IsSurvivalActive() const { return m_ePhase == Phase::Survival || m_ePhase == Phase::Ready; }

	// 게임 종료(탈출 성공) 도달 1회 소비 — GameLoop이 방을 로비 상태로 되돌리는 트리거
	bool ConsumeEnded()
	{
		if (m_ePhase != Phase::Ended) return false;
		m_ePhase = Phase::None;
		return true;
	}

private:
	Phase             m_ePhase = Phase::None;
	DWORD             m_dwSurviveStart = 0;    // 서바이벌 시작 시각(ms) = 착륙 컷씬(정지) 종료 후
	DWORD             m_dwLastBroadcast = 0;   // 상태 브로드캐스트 스로틀
	std::atomic<bool> m_bRequested{ false };   // 탈출 키 입력됨(누구든). 틱 스레드가 소비.
};
