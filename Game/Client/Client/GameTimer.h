#pragma once

const ULONG MAX_SAMPLE_COUNT = 50; // Maximum frame time sample count

class GameTimer {

	DECLARE_SINGLE(GameTimer);

public:
	void Initialize();

	void Tick(float fLockFPS = 0.0f);
	void Start();
	void Stop();
	void Reset();

	float GetTimeElapsed();
	double GetTotalTime();

	unsigned long GetFrameRate(const std::wstring& wsvGameName, std::wstring& wstrString);

private:
	double							m_dTimeScale;
	double							m_dTotalTimeElapsed;

	__int64							m_nBasePerformanceCounter;
	__int64							m_nPausedPerformanceCounter;
	__int64							m_nStopPerformanceCounter;
	__int64							m_nCurrentPerformanceCounter;
	__int64							m_nLastPerformanceCounter;

	__int64							m_nPerformanceFrequencyPerSec;

	double							m_dFrameTime[MAX_SAMPLE_COUNT];
	ULONG							m_nSampleCount;

	unsigned long					m_nCurrentFrameRate;
	unsigned long					m_nFramesPerSecond;
	double							m_dFPSTimeElapsed;

	bool							m_bStopped;
};

