#pragma once

#define WIN32_LEAN_AND_MEAN

#ifdef _DEBUG
#pragma comment(lib, "Debug\\ServerCore.lib")
#else
#pragma comment(lib, "Release\\ServerCore.lib")
#endif

#include "CorePch.h"

#pragma comment(lib, "ws2_32.lib")

constexpr int32 SEND_BUF_SIZE = 1024;
constexpr int32 RECV_BUF_SIZE = 1024;

constexpr int32 SERVERPORT = 9000;

enum IO_TYPE
{
	READ,
	WRITE,
	ACCEPT,
	CONNECT,
};

struct OverlappedEx
{
	WSAOVERLAPPED overlapped = {};
	int32 type = 0;	// read, write, accept, connect ...
};
