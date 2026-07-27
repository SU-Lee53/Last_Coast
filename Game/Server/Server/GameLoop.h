#pragma once
#include "pch.h"

// ─────────────────────────────────────────────────────────────────────────────
// 게임 틱 스레드 (30Hz) — 활성 방들을 순회하며 방별 GameWorld 를 틱:
// 좀비 AI/스폰 + 상태 브로드캐스트 + 체크포인트/탈출 + 종료 후 로비 복귀.
// Run() 은 SHARED->IsRunning() 동안 블로킹.
// ─────────────────────────────────────────────────────────────────────────────
class GameLoop {
	DECLARE_SINGLE(GameLoop)
public:
	void Run(); // 전용 스레드에서 호출

private:
	GameLoop() = default;
};
