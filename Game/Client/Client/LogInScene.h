#pragma once
#include "Scene.h"

class InputTextBox;

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

	std::shared_ptr<InputTextBox> m_pServerIPInputBox;
	std::shared_ptr<InputTextBox> m_pIDInputBox;
	std::shared_ptr<InputTextBox> m_pPWInputBox;
	std::shared_ptr<TextBox> m_pResultText;

	bool m_bLastRegisterTry = false;
	bool m_bLastLogInTry = false;
	bool m_bConnectionResultHandled = true;

	bool m_bProceed = false;
};
