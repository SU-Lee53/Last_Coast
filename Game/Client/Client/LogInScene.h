#pragma once
#include "Scene.h"

class LogInScene : public Scene {
public:
	void BuildObjects() override;
	void OnEnterScene() override;
	void OnLeaveScene() override;
	void ProcessInput() override;
	void Update() override;

	bool TryLogIn();
	bool TryRegister();

private:
	std::string m_strServerIPInput;
	std::string m_strIDInput;
	std::string m_strPasswordInput;

	bool m_bLastRegisterTry = false;
	bool m_bLastLogInTry = false;
};
