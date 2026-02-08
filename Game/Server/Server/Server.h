#pragma once
#include "NetworkManager.h"


class Server
{
public:
	void Init();
	void Start();
	void End();

private:
	NetworkManager m_networkManager;
};

