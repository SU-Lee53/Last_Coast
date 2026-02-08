#include "pch.h"
#include "ThreadManager.h"
#include "NetworkManager.h"
#include "Session.h"
#include "Server.h"

int main()
{
	Server server;

	server.Init();
	server.Start();

	GThreadManager->Join();

	::WSACleanup();
}
