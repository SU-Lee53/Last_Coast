#pragma once
#include "pch.h"

struct Session
{
	SOCKET socket = INVALID_SOCKET;
	char recvBuffer[RECV_BUF_SIZE] = {};
	int32 recvBytes = 0;
};

