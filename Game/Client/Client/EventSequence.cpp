#include "pch.h"
#include "EventSequence.h"


void EventSequence::Initialize()
{
	if (!m_pCurScene) {
		return;
	}

	for (auto& pEvent : m_pEvents) {
		pEvent->Initialize(m_pCurScene);
	}
}

void EventSequence::Update()
{
	if (!m_pCurScene) {
		return;
	}

	for (auto& pEvent : m_pEvents) {
		pEvent->Update(m_pCurScene);
	}
}

void EventSequence::ShowDebugOptions()
{
	for (auto& pEvent : m_pEvents) {
		pEvent->ShowDebugOptions();
	}
}
