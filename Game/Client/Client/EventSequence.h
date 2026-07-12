#pragma once
#include "GameEvent.h"

class EventSequence {
public:
	EventSequence(Scene* pScene) :m_pCurScene {pScene} {}

	void Initialize();
	void Update();
	void ShowDebugOptions();

	// 이벤트 내부(Update 순회 중)에서도 안전하게 후속 이벤트 추가 가능 —
	// 바로 m_pEvents에 넣지 않고 pending에 쌓았다가 다음 Initialize/Update 시작 시 합류.
	void AddEvent(const std::shared_ptr<IGameEvent>& pEvent) { m_pPendingEvents.push_back(pEvent); }

private:
	void FlushPendingEvents();

private:
	std::vector<std::shared_ptr<IGameEvent>> m_pEvents;
	std::vector<std::shared_ptr<IGameEvent>> m_pPendingEvents;
	Scene* m_pCurScene;	// not weak_ptr bc Scene is managed by raw in SceneManager

};

