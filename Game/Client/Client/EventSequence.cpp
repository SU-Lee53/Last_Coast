#include "pch.h"
#include "EventSequence.h"


void EventSequence::FlushPendingEvents()
{
	if (m_pPendingEvents.empty()) {
		return;
	}

	m_pEvents.insert(m_pEvents.end(), m_pPendingEvents.begin(), m_pPendingEvents.end());
	m_pPendingEvents.clear();
}

void EventSequence::Initialize()
{
	if (!m_pCurScene) {
		return;
	}

	FlushPendingEvents();

	for (auto& pEvent : m_pEvents) {
		pEvent->Initialize(m_pCurScene);
	}
}

void EventSequence::Update()
{
	if (!m_pCurScene) {
		return;
	}

	FlushPendingEvents();

	for (auto& pEvent : m_pEvents) {
		pEvent->Update(m_pCurScene);
	}

	// 완료된 1회성 이벤트 정리 (서버 트리거로 런타임 추가된 이벤트 등)
	std::erase_if(m_pEvents, [](const std::shared_ptr<IGameEvent>& pEvent) {
		return pEvent->IsFinished();
	});
}

void EventSequence::ShowDebugOptions()
{
	for (auto& pEvent : m_pEvents) {
		pEvent->ShowDebugOptions();
	}
}
