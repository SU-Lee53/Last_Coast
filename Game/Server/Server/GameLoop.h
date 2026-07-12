#pragma once
#include "pch.h"

// ─────────────────────────────────────────────────────────────────────────────
// 게임 틱 스레드 (30Hz) — 좀비 AI 업데이트/스폰 + 상태 브로드캐스트 +
// 체크포인트/탈출 시퀀스 진행. Run() 은 WORLD->IsRunning() 동안 블로킹.
// ─────────────────────────────────────────────────────────────────────────────
class GameLoop {
	DECLARE_SINGLE(GameLoop)
public:
	void Run(); // 전용 스레드에서 호출

private:
	GameLoop() = default;
};
