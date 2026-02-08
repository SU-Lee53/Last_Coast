#include "pch.h"
#include "Server.h"

void Server::Init()
{
	m_networkManager.Init();
}

void Server::Start()
{
	m_networkManager.Accept();
}

void Server::End()
{
	
}
