#include "pch.h"
#include "NetworkManager.h"
#include "Packets.h"

HANDLE NetworkManager::g_hNetworkThread = nullptr;
ConnectState m_connectState = ConnectState::None;

NetworkManager::~NetworkManager()
{
	Disconnect();
}

void NetworkManager::Initialize()
{
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		__debugbreak();
		PostQuitMessage(0);
	}

	m_hClientSocket = WSASocket(AF_INET, SOCK_STREAM, 0, 0, 0, WSA_FLAG_OVERLAPPED);
	if (m_hClientSocket == INVALID_SOCKET)
	{
		__debugbreak();
		PostQuitMessage(0);
	}
}

void NetworkManager::ConnectToServer()
{
	ImGui::Begin("NetworkManager::ConnectToServer()");
	{
		const char* cstrConnectionStatus = m_bConnected ? "Connected" : "Not connected yet";
		ImGui::Text(cstrConnectionStatus);
		ImGui::InputText("Server IP", m_cstrServerIP, IM_ARRAYSIZE(m_cstrServerIP));

		if (ImGui::Button("Try Connect")) {
			sockaddr_in serveraddr;
			memset(&serveraddr, 0, sizeof(serveraddr));
			serveraddr.sin_family = AF_INET;
			serveraddr.sin_port = htons(SERVERPORT);
			inet_pton(AF_INET, m_cstrServerIP, &serveraddr.sin_addr);
			int retval = WSAConnect(m_hClientSocket, reinterpret_cast<sockaddr*>(&serveraddr), sizeof(serveraddr), 0, 0, 0, 0);

			if (retval == SOCKET_ERROR)
			{
				int err = WSAGetLastError();

				if (err == WSAEWOULDBLOCK)
				{
					m_connectState = ConnectState::Connecting;
					m_strErrorLog = "Connecting...";
				}
				else
				{
					m_connectState = ConnectState::Failed;
					m_strErrorLog = err_display("connect()");
				}
			}
			else
			{
				m_connectState = ConnectState::Connected;
				m_bConnected = true;
				m_bOfflineMode = false;
				SendLoginPacket();
				g_hNetworkThread = CreateThread(NULL, 0, ProcessNetwork, this, 0, NULL);
			}
		}
		ImGui::Text(m_strErrorLog.c_str());

		if (ImGui::Button("Offline Mode")) {
			m_bConnected = true;
			m_bGameBegin = true;
		}

		if (m_bConnected) {
			ImGui::Text("Wait for game start...");
		}

		if (m_connectState == ConnectState::Connecting)
		{
			fd_set writeSet;
			FD_ZERO(&writeSet);
			FD_SET(m_hClientSocket, &writeSet);

			timeval timeout{};
			timeout.tv_sec = 0;
			timeout.tv_usec = 0;

			int ret = select(0, nullptr, &writeSet, nullptr, &timeout);

			if (ret > 0)
			{
				int err = 0;
				int len = sizeof(err);

				getsockopt(m_hClientSocket, SOL_SOCKET, SO_ERROR, (char*)&err, &len);

				if (err == 0)
				{
					// 진짜 연결 성공
					m_connectState = ConnectState::Connected;
					m_bConnected = true;
					m_bOfflineMode = false;
					m_strErrorLog = "Connected!";
					SendLoginPacket();
					g_hNetworkThread = CreateThread(NULL, 0, ProcessNetwork, this, 0, NULL);
				}
				else
				{
					// 연결 실패
					m_connectState = ConnectState::Failed;
					m_strErrorLog = "Connect Failed";
				}
			}
		}

		switch (m_connectState)
		{
		case ConnectState::None:
			ImGui::Text("Not connected");
			break;
		case ConnectState::Connecting:
			ImGui::Text("Connecting...");
			break;
		case ConnectState::Connected:
			ImGui::Text("Connected");
			break;
		case ConnectState::Failed:
			ImGui::Text("Failed");
			break;
		}
	}
	ImGui::End();
}

void NetworkManager::Disconnect()
{
	m_bConnected = false;
	CloseHandle(g_hNetworkThread);
	closesocket(m_hClientSocket);
	WSACleanup();
}

// 일단 키 입력만
void NetworkManager::SendLoginPacket()
{
	C2S_Login packet;
	packet.size = sizeof(C2S_Login);
	packet.type = C2S_LOGIN;
	strncpy_s(packet.username, "Player", sizeof(packet.username));

	WSABUF wsa;
	wsa.buf = reinterpret_cast<char*>(&packet);
	wsa.len = packet.size;
	WSASend(m_hClientSocket, &wsa, 1, nullptr, 0, nullptr, nullptr);
}

void NetworkManager::SendData()
{
	m_SendMovePacket.size = sizeof(C2S_Move);
	m_SendMovePacket.type = C2S_MOVE;
	m_SendMovePacket.dir  = UP;

	if (INPUT->GetButtonDown('W')) m_SendMovePacket.dir = UP;
	if (INPUT->GetButtonDown('S')) m_SendMovePacket.dir = DOWN;
	if (INPUT->GetButtonDown('A')) m_SendMovePacket.dir = LEFT;
	if (INPUT->GetButtonDown('D')) m_SendMovePacket.dir = RIGHT;

	m_wsabuf.buf = reinterpret_cast<char*>(&m_SendMovePacket);
	m_wsabuf.len = sizeof(m_SendMovePacket);
	WSASend(m_hClientSocket, &m_wsabuf, 1, 0, 0, &m_over, send_callback);
}

void NetworkManager::ReceiveData()
{
	DWORD flags = 0;

	m_wsabuf.buf = m_Buffer;
	m_wsabuf.len = BUF_SIZE;

	ZeroMemory(&m_over, sizeof(m_over));

	int ret = WSARecv(m_hClientSocket, &m_wsabuf, 1, nullptr, &flags, &m_over, recv_callback);

	if (ret == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != WSA_IO_PENDING)
		{
			printf("WSARecv error: %d\n", err);
		}
	}

}

void NetworkManager::send_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED over, DWORD flags)
{
	NetworkManager* N = NetworkManager::GetInstance();
	N->m_over = *over;
	memset(over, 0, sizeof(*over));
	N->ReceiveData();
}

void NetworkManager::recv_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED over, DWORD flags)
{
	NetworkManager* N = NetworkManager::GetInstance();

	if (err != 0 || num_bytes == 0)
	{
		N->Disconnect();
		return;
	}

	N->m_over = *over;
	N->ProcessPackets(static_cast<int>(num_bytes));
	N->SendData();
}

// -----------------------------------------------------------------------
// 패킷 재조립 + 디스패치
// -----------------------------------------------------------------------

void NetworkManager::ProcessPackets(int numBytes)
{
	// 수신된 바이트를 재조립 버퍼에 추가
	if (m_nRecvPending + numBytes > static_cast<int>(sizeof(m_RecvBuf)))
	{
		m_nRecvPending = 0; // 버퍼 오버플로 — 버림
		return;
	}
	memcpy(m_RecvBuf + m_nRecvPending, m_Buffer, numBytes);
	m_nRecvPending += numBytes;

	// 완성된 패킷을 순서대로 처리
	int nConsumed = 0;
	while (m_nRecvPending - nConsumed >= 1)
	{
		unsigned char nPacketSize = static_cast<unsigned char>(m_RecvBuf[nConsumed]);
		if (nPacketSize == 0 || m_nRecvPending - nConsumed < nPacketSize) break;

		ProcessSinglePacket(m_RecvBuf + nConsumed, nPacketSize);
		nConsumed += nPacketSize;
	}

	// 미처리 바이트를 버퍼 앞으로 이동
	m_nRecvPending -= nConsumed;
	if (m_nRecvPending > 0)
		memmove(m_RecvBuf, m_RecvBuf + nConsumed, m_nRecvPending);
}

void NetworkManager::ProcessSinglePacket(const char* data, int size)
{
	if (size < 2) return;
	PACKET_TYPE type = *reinterpret_cast<const PACKET_TYPE*>(data + 1);

	switch (type)
	{
	case S2C_LOGIN_RESULT:
	{
		if (size < static_cast<int>(sizeof(S2C_LoginResult))) return;
		auto* p = reinterpret_cast<const S2C_LoginResult*>(data);
		if (p->success) m_bGameBegin = true;
		break;
	}
	case S2C_AVATAR_INFO:
	{
		if (size < static_cast<int>(sizeof(S2C_AvatarInfo))) return;
		auto* p = reinterpret_cast<const S2C_AvatarInfo*>(data);
		m_nPlayerID = p->playerId;
		break;
	}
	case S2C_ADD_PLAYER:
	{
		if (size < static_cast<int>(sizeof(S2C_AddPlayer))) return;
		auto* p = reinterpret_cast<const S2C_AddPlayer*>(data);
		std::lock_guard<std::mutex> lock(m_Mutex);
		m_RemotePlayers[p->playerId].active = true;
		break;
	}
	case S2C_REMOVE_PLAYER:
	{
		if (size < static_cast<int>(sizeof(S2C_RemovePlayer))) return;
		auto* p = reinterpret_cast<const S2C_RemovePlayer*>(data);
		std::lock_guard<std::mutex> lock(m_Mutex);
		m_RemotePlayers.erase(p->playerId);
		break;
	}
	case S2C_MOVE_PLAYER:
	{
		if (size < static_cast<int>(sizeof(S2C_MovePlayer))) return;
		auto* p = reinterpret_cast<const S2C_MovePlayer*>(data);

		NetSnapshot snap;
		snap.pos  = Vector3(ServerToWorld(p->x), 0.f, ServerToWorld(p->y));
		snap.time = GetNetTimeSec();

		std::lock_guard<std::mutex> lock(m_Mutex);
		auto& state = m_RemotePlayers[p->playerId];
		state.active = true;
		state.snapshots.push_back(snap);
		if (state.snapshots.size() > RemotePlayerState::MAX_SNAPSHOTS)
			state.snapshots.pop_front();
		break;
	}
	case S2C_SPAWN_ZOMBIE:
	{
		if (size < static_cast<int>(sizeof(S2C_SpawnZombie))) return;
		auto* p = reinterpret_cast<const S2C_SpawnZombie*>(data);
		SpawnEvent ev{ p->zombieId, Vector3(p->x, p->y, p->z) };
		std::lock_guard<std::mutex> lock(m_Mutex);
		m_PendingSpawns.push_back(ev);
		break;
	}
	case S2C_DESPAWN_ZOMBIE:
	{
		if (size < static_cast<int>(sizeof(S2C_DespawnZombie))) return;
		auto* p = reinterpret_cast<const S2C_DespawnZombie*>(data);
		std::lock_guard<std::mutex> lock(m_Mutex);
		m_PendingDespawns.push_back(p->zombieId);
		m_ZombieStates.erase(p->zombieId);
		break;
	}
	case S2C_ZOMBIE_STATE:
	{
		if (size < static_cast<int>(sizeof(S2C_ZombieState))) return;
		auto* p = reinterpret_cast<const S2C_ZombieState*>(data);
		ZombieServerState state;
		state.x             = p->x;
		state.z             = p->z;
		state.yaw           = p->yaw;
		state.waypointX     = p->waypointX;
		state.waypointZ     = p->waypointZ;
		state.behaviorState = p->behaviorState;
		state.receivedTime  = GetNetTimeSec();
		state.valid         = true;
		std::lock_guard<std::mutex> lock(m_Mutex);
		m_ZombieStates[p->zombieId] = state;
		break;
	}
	case S2C_ZOMBIE_ATTACK:
	{
		if (size < static_cast<int>(sizeof(S2C_ZombieAttack))) return;
		auto* p = reinterpret_cast<const S2C_ZombieAttack*>(data);
		AttackEvent ev{ p->zombieId, p->targetPlayerId, p->damage };
		std::lock_guard<std::mutex> lock(m_Mutex);
		m_PendingAttacks.push_back(ev);
		break;
	}
	default:
		break;
	}
}

// -----------------------------------------------------------------------
// 플레이어 위치 전송 (Task 5)
// -----------------------------------------------------------------------

void NetworkManager::SetLocalPlayerInfo(const Vector3& pos, float yaw)
{
	m_v3LocalPlayerPos = pos;
	m_fLocalPlayerYaw  = yaw;
}

void NetworkManager::TrySendPlayerPosition()
{
	if (!m_bConnected || m_bOfflineMode) return;

	float fNow = GetNetTimeSec();
	if (fNow - m_fLastPosSendTime < POS_SEND_INTERVAL) return;
	m_fLastPosSendTime = fNow;

	C2S_PlayerPosition p;
	p.size = sizeof(C2S_PlayerPosition);
	p.type = C2S_PLAYER_POSITION;
	p.x    = m_v3LocalPlayerPos.x;
	p.y    = m_v3LocalPlayerPos.y;
	p.z    = m_v3LocalPlayerPos.z;
	p.yaw  = m_fLocalPlayerYaw;

	// 소켓이 non-blocking overlapped 이므로 WSASend 사용 (nullptr overlapped = best-effort)
	WSABUF wsa;
	wsa.buf = reinterpret_cast<char*>(&p);
	wsa.len = p.size;
	WSASend(m_hClientSocket, &wsa, 1, nullptr, 0, nullptr, nullptr);
}

// -----------------------------------------------------------------------
// 좀비 이벤트 소비 (Task 6/7/9)
// -----------------------------------------------------------------------

std::vector<SpawnEvent> NetworkManager::ConsumeSpawnEvents()
{
	std::vector<SpawnEvent> out;
	std::lock_guard<std::mutex> lock(m_Mutex);
	out.swap(m_PendingSpawns);
	return out;
}

std::vector<int> NetworkManager::ConsumeDespawnEvents()
{
	std::vector<int> out;
	std::lock_guard<std::mutex> lock(m_Mutex);
	out.swap(m_PendingDespawns);
	return out;
}

std::vector<AttackEvent> NetworkManager::ConsumeAttackEvents()
{
	std::vector<AttackEvent> out;
	std::lock_guard<std::mutex> lock(m_Mutex);
	out.swap(m_PendingAttacks);
	return out;
}

bool NetworkManager::GetLatestZombieState(int zombieId, ZombieServerState& outState) const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	auto it = m_ZombieStates.find(zombieId);
	if (it == m_ZombieStates.end() || !it->second.valid) return false;
	outState = it->second;
	return true;
}

// -----------------------------------------------------------------------
// 보간 / 데드 레커닝
// -----------------------------------------------------------------------

float NetworkManager::GetNetTimeSec()
{
	static ULONGLONG nEpoch = GetTickCount64();
	return static_cast<float>(GetTickCount64() - nEpoch) * 0.001f;
}

void NetworkManager::UpdateInterpolation()
{
	float fRenderTime = GetNetTimeSec() - INTERP_DELAY;

	std::lock_guard<std::mutex> lock(m_Mutex);
	for (auto& [nId, state] : m_RemotePlayers)
	{
		const auto& snaps = state.snapshots;
		if (snaps.empty()) continue;

		// 렌더 시각을 감싸는 두 스냅샷 탐색 → 선형 보간
		bool bFound = false;
		for (int i = static_cast<int>(snaps.size()) - 1; i >= 1; --i)
		{
			const auto& s0 = snaps[i - 1];
			const auto& s1 = snaps[i];
			if (s0.time <= fRenderTime && fRenderTime <= s1.time)
			{
				float fSpan = s1.time - s0.time;
				float t     = (fSpan > 0.f) ? (fRenderTime - s0.time) / fSpan : 1.f;
				state.interpolatedPos = Vector3::Lerp(s0.pos, s1.pos, t);
				bFound = true;
				break;
			}
		}

		if (bFound) continue;

		if (fRenderTime < snaps.front().time)
		{
			// 아직 버퍼 지연 전 — 가장 오래된 스냅샷 사용
			state.interpolatedPos = snaps.front().pos;
		}
		else
		{
			// 스냅샷보다 렌더 시각이 앞서 있음 (패킷 지연/손실)
			// → 데드 레커닝: 마지막 두 스냅샷의 속도로 외삽 (최대 0.5초)
			if (snaps.size() >= 2)
			{
				const auto& s0 = snaps[snaps.size() - 2];
				const auto& s1 = snaps.back();
				float fSpan = s1.time - s0.time;
				if (fSpan > 0.f)
				{
					Vector3 v3Vel  = (s1.pos - s0.pos) * (1.f / fSpan);
					float   fExtrap = min(fRenderTime - s1.time, 0.5f);
					state.interpolatedPos = s1.pos + v3Vel * fExtrap;
				}
				else
				{
					state.interpolatedPos = snaps.back().pos;
				}
			}
			else
			{
				state.interpolatedPos = snaps.back().pos;
			}
		}
	}
}

bool NetworkManager::GetInterpolatedPosition(int playerId, Vector3& outPos) const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	auto it = m_RemotePlayers.find(playerId);
	if (it == m_RemotePlayers.end() || !it->second.active) 
		return false;
	outPos = it->second.interpolatedPos;
	return true;
}

DWORD WINAPI NetworkManager::ProcessNetwork(LPVOID arg)
{
	NetworkManager* self = reinterpret_cast<NetworkManager*>(arg);

	self->ReceiveData();

	while (self->m_bConnected)
		SleepEx(100, true);

	self->Disconnect();

	return 0;
}
