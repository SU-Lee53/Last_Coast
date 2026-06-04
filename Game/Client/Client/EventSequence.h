#pragma once
#include "GameEvent.h"

class EventSequence {
public:
	EventSequence(Scene* pScene) :m_pCurScene {pScene} {}

	void Initialize();
	void Update();
	void ShowDebugOptions();

	void AddEvent(const std::shared_ptr<IGameEvent>& pEvent) { m_pEvents.push_back(pEvent); }

private:
	std::vector<std::shared_ptr<IGameEvent>> m_pEvents;
	Scene* m_pCurScene;	// not weak_ptr bc Scene is managed by raw in SceneManager

};

