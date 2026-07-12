#pragma once
#include "pch.h"
#include "Session.h"

// ─────────────────────────────────────────────────────────────────────────────
// IOCP 네트워크 계층 — 리슨 소켓/억셉트/워커 스레드 루프/세션 해제.
//   Init() 후 WorkerLoop() 를 워커 스레드 여러 개가 동시에 실행한다.
// ─────────────────────────────────────────────────────────────────────────────
class Network {
	DECLARE_SINGLE(Network)
public:
	bool Init(short port); // WSAStartup + 리슨 소켓 + IOCP 생성 + 첫 AcceptEx
	void WorkerLoop();     // GQCS 루프 — 반환하지 않음
	void Shutdown();

private:
	Network() = default;

	void HandleAccept(EXP_OVER& over);
	void Disconnect(int id);

	SOCKET   m_listenSocket = INVALID_SOCKET;
	HANDLE   m_iocp = nullptr;
	EXP_OVER m_acceptOver{ IO_ACCEPT };
};
