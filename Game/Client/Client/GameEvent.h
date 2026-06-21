#pragma once


/*
	Event 종류?
	1. 시간 변화 -> 성공
	2. Cinematic Camera 의 이동 (컷씬 재생용) -> 성공 (나중에 CatmullRom -> Hermite 기반으로 변경)
	3. 특정 오브젝트의 소환/이동 (컷씬 재생용)
	4. 파티클 소환 -> 성공
	5. 파라미터 변경
		5-1. Tone
		5-2. Fx (Grading, Vignette)
		5-3. Skybox
		5-4. 더?
*/

interface IGameEvent abstract {
public:
	virtual void Initialize(Scene* pScene) {};
	virtual void Update(Scene* pScene) = 0;

	virtual void OnEnterEvent(Scene* pScene) {};
	virtual void OnUpdateEvent(Scene* pScene) {};
	virtual void OnLeaveEvent(Scene* pScene) {};

	virtual void ShowDebugOptions() { };

protected:
	bool m_bIsTriggered = false;

};

interface ILoopEvent abstract : public IGameEvent {
public:
	virtual void Update(Scene* pScene) override;

};

interface IIntervalEvent abstract : public IGameEvent {
public:
	virtual void Update(Scene* pScene) override;

protected:
	float m_fInterval = 0.f;
	float m_fTimeElapsed = 0.f;
};

interface ITriggerEvent abstract : public IGameEvent {
public:
	virtual void Update(Scene* pScene) override;

	void SetBeginTrigger(const std::function<bool()>& fnTrigger) { m_fnBeginTrigger = fnTrigger; }
	void SetEndTrigger(const std::function<bool()>& fnTrigger) { m_fnEndTrigger= fnTrigger; }

protected:
	std::function<bool()> m_fnBeginTrigger;
	std::function<bool()> m_fnEndTrigger;
};

// Test
class TimeForwardEvent : public ILoopEvent {
public:
	virtual void OnUpdateEvent(Scene* pScene) override;

private:
	float m_fTime = 0.f;

};

// Test
class BleedEvent : public IIntervalEvent {
public:
	virtual void Initialize(Scene* pScene) override;
	virtual void OnUpdateEvent(Scene* pScene) override;

private:

};

class CinematicCameraEvent : public ITriggerEvent {
public:
	virtual void Initialize(Scene* pScene) override;
	virtual void OnEnterEvent(Scene* pScene) override;
	virtual void OnUpdateEvent(Scene* pScene) override;
	virtual void OnLeaveEvent(Scene* pScene) override;

private:
	float m_fTotalPlayTime = 30.f;
	float m_fTimeElapsed = 0.f;

	bool m_bPlayed = false;
	std::vector<Vector3> m_v3ControlPoints;
	std::shared_ptr<Camera> m_pCinematicCamera;
	std::shared_ptr<Camera> m_pCameraSwapped;
};

// 1. 
