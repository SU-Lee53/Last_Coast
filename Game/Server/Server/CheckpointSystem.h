#pragma once
#include "pch.h"

class GameWorld;

// ─────────────────────────────────────────────────────────────────────────────
// 체크포인트(전원 도달 트리거)
//   맵이 일직선(+X 전진)이라 진행도 = pos.x. 모든 연결 플레이어의 min(x)가
//   체크포인트 x를 넘으면(= 가장 뒤처진 플레이어까지 도달) 1회 events 브로드캐스트.
// ─────────────────────────────────────────────────────────────────────────────
struct EventEmit {
	int     eventId;          // GameEventId
	Vector3 pos;              // 위치 의존 이벤트용(폭발 등). 무관하면 {}
	float   fTargetValue;
	float   fDuration;
	int     presetId;         // EnvironmentPresetId (GE_ENVIRONMENT 전용)
	float   fFreezeDuration;  // 컷씬 길이(초). >0 이면 그 동안 서버 좀비 AI 정지(온라인 스냅 방지). 비컷씬=0

	EventEmit(int eventId_, const Vector3& pos_, float fTargetValue_,
		float fDuration_, int presetId_, float fFreezeDuration_)
		: eventId(eventId_), pos(pos_), fTargetValue(fTargetValue_)
		, fDuration(fDuration_), presetId(presetId_), fFreezeDuration(fFreezeDuration_) {
	}
};

struct Checkpoint {
	Vector3                v3Pos;          // 익스포트 위치(cm). x 좌표를 임계값으로 사용
	std::vector<EventEmit> events;         // 도달 시 발사할 이벤트들
	bool                   bFired = false; // 1회성
};

class CheckpointSystem {
public:
	// GAME_Checkpoints.json("Checkpoints") 에서 체크포인트 위치 로드
	// (언리얼 SaveCheckpointsToJson 출력, 익스포터가 이름 끝 숫자순 정렬).
	int  Load(const std::string& path);

	// 체크포인트 index → 이벤트 배정 테이블 (위치는 익스포트 순서 = Checkpoint_1, Checkpoint_2 …)
	void AssignEvents();

	// 도달 판정 + 이벤트 발사 — 게임 틱 스레드 전용.
	// 컷씬 정지/탈출 시퀀스 시작은 world 를 통해 전파한다.
	void Update(float minPlayerX, DWORD now, GameWorld& world);

	bool IsEmpty() const { return m_Checkpoints.empty(); }

	// 발동된 체크포인트 중 가장 전방(x 최대) 위치. 없으면 nullptr — 부활 위치 판정용.
	const Vector3* GetLastFiredPos() const;

private:
	std::vector<Checkpoint> m_Checkpoints;
};
