#include "pch.h"
#include "GameTimer.h"

void GameTimer::Initialize()
{
	::QueryPerformanceFrequency((LARGE_INTEGER*)&m_nPerformanceFrequencyPerSec);
	::QueryPerformanceCounter((LARGE_INTEGER*)&m_nLastPerformanceCounter);
	m_dTimeScale = 1.0 / (double)m_nPerformanceFrequencyPerSec;

	m_nBasePerformanceCounter = m_nLastPerformanceCounter;
	m_nPausedPerformanceCounter = 0;
	m_nStopPerformanceCounter = 0;

	m_nSampleCount = 0;
	m_nCurrentFrameRate = 0;
	m_nFramesPerSecond = 0;
	m_dFPSTimeElapsed = 0.0f;
}
void GameTimer::Tick(float fLockFPS)
{
	if (m_bStopped)
	{
		m_dTimeElapsed = 0.0f;
		return;
	}
	float fTimeElapsed;

	::QueryPerformanceCounter((LARGE_INTEGER*)&m_nCurrentPerformanceCounter);
	fTimeElapsed = float((m_nCurrentPerformanceCounter - m_nLastPerformanceCounter) * m_dTimeScale);

	if (fLockFPS > 0.0f)
	{
		while (fTimeElapsed < (1.0f / fLockFPS))
		{
			::QueryPerformanceCounter((LARGE_INTEGER*)&m_nCurrentPerformanceCounter);
			fTimeElapsed = float((m_nCurrentPerformanceCounter - m_nLastPerformanceCounter) * m_dTimeScale);
		}
	}

	m_nLastPerformanceCounter = m_nCurrentPerformanceCounter;

	if (fabsf(fTimeElapsed - m_dTimeElapsed) < 1.0f)
	{
		::memmove(&m_dFrameTime[1], m_dFrameTime, (MAX_SAMPLE_COUNT - 1) * sizeof(float));
		m_dFrameTime[0] = fTimeElapsed;
		if (m_nSampleCount < MAX_SAMPLE_COUNT) m_nSampleCount++;
	}

	m_nFramesPerSecond++;
	m_dFPSTimeElapsed += fTimeElapsed;
	if (m_dFPSTimeElapsed > 1.0f)
	{
		m_nCurrentFrameRate = m_nFramesPerSecond;
		m_nFramesPerSecond = 0;
		m_dFPSTimeElapsed = 0.0f;
	}

	m_dTimeElapsed = 0.0f;
	for (ULONG i = 0; i < m_nSampleCount; i++) m_dTimeElapsed += m_dFrameTime[i];
	if (m_nSampleCount > 0) m_dTimeElapsed /= m_nSampleCount;
}

unsigned long GameTimer::GetFrameRate(const std::wstring& wsvGameName, std::wstring& wstrString)
{
	wstrString = std::format(L"{} ({} FPS)", wsvGameName, m_nCurrentFrameRate);

	return m_nCurrentFrameRate;
}

float GameTimer::GetTimeElapsed()
{
	return m_dTimeElapsed;
}

double GameTimer::GetTotalTime()
{
	if (m_bStopped) return(float(((m_nStopPerformanceCounter - m_nPausedPerformanceCounter) - m_nBasePerformanceCounter) * m_dTimeScale));
	return float(((m_nCurrentPerformanceCounter - m_nPausedPerformanceCounter) - m_nBasePerformanceCounter) * m_dTimeScale);
}

void GameTimer::Reset()
{
	__int64 nPerformanceCounter;
	::QueryPerformanceCounter((LARGE_INTEGER*)&nPerformanceCounter);

	m_nBasePerformanceCounter = nPerformanceCounter;
	m_nLastPerformanceCounter = nPerformanceCounter;
	m_nStopPerformanceCounter = 0;
	m_bStopped = false;
}

void GameTimer::Start()
{
	__int64 nPerformanceCounter;
	::QueryPerformanceCounter((LARGE_INTEGER*)&nPerformanceCounter);
	if (m_bStopped)
	{
		m_nPausedPerformanceCounter += (nPerformanceCounter - m_nStopPerformanceCounter);
		m_nLastPerformanceCounter = nPerformanceCounter;
		m_nStopPerformanceCounter = 0;
		m_bStopped = false;
	}
}

void GameTimer::Stop()
{
	if (!m_bStopped)
	{
		::QueryPerformanceCounter((LARGE_INTEGER*)&m_nStopPerformanceCounter);
		m_bStopped = true;
	}
}
