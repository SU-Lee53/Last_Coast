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
	if (m_hClientSocket == INVALID_SOCKET || !m_bConnected)
		return;

	C2S_Login packet;
	packet.size = sizeof(C2S_Login);
	packet.type = C2S_LOGIN;
	strncpy_s(packet.username, "Player", sizeof(packet.username));

	DWORD dwSent = 0;
	WSABUF wsa;
	wsa.buf = reinterpret_cast<char*>(&packet);
	wsa.len = packet.size;
	WSASend(m_hClientSocket, &wsa, 1, &dwSent, 0, nullptr, nullptr);
}

void NetworkManager::SendData()
{
	//m_SendMovePacket.size = sizeof(C2S_Move);
	//m_SendMovePacket.type = C2S_MOVE;
	//m_SendMovePacket.dir = UP;

	//if (INPUT->GetButtonDown('W')) m_SendMovePacket.dir = UP;
	//if (INPUT->GetButtonDown('S')) m_SendMovePacket.dir = DOWN;
	//if (INPUT->GetButtonDown('A')) m_SendMovePacket.dir = LEFT;
	//if (INPUT->GetButtonDown('D')) m_SendMovePacket.dir = RIGHT;

	//m_wsabuf.buf = reinterpret_cast<char*>(&m_SendMovePacket);
	//m_wsabuf.len = sizeof(m_SendMovePacket);
	//WSASend(m_hClientSocket, &m_wsabuf, 1, 0, 0, &m_over, send_callback);
}

struct SendContext {
	WSAOVERLAPPED over;
	WSABUF wsabuf;
	char buffer[256];
};

void NetworkManager::SendPacket(void* packet, int size)
{
	if (!m_bConnected) return;

	SendContext* ctx = new SendContext();
	ZeroMemory(&ctx->over, sizeof(WSAOVERLAPPED));
	memcpy(ctx->buffer, packet, size);
	ctx->wsabuf.buf = ctx->buffer;
	ctx->wsabuf.len = size;

	int ret = WSASend(m_hClientSocket, &ctx->wsabuf, 1, nullptr, 0, &ctx->over, send_callback);
	if (ret == SOCKET_ERROR) {
		int err = WSAGetLastError();
		if (err != WSA_IO_PENDING) {
			char szError[256];
			sprintf_s(szError, "WSASend Failed: %d\n", err);
			OutputDebugStringA(szError);
			delete ctx;
		}
	}
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
	// WSASend를 비동기로 호출했을 때 완료 처리
	SendContext* ctx = reinterpret_cast<SendContext*>(over);
	delete ctx;
}

void NetworkManager::recv_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED over, DWORD flags)
{
	NetworkManager* N = NetworkManager::GetInstance();
	
	if (num_bytes == 0 || err != 0) {
		N->Disconnect();
		return;
	}
	//// 다시 수신 대기 (핑퐁 제거, 계속해서 Recv만 돌림)
	N->ProcessPackets(static_cast<int>(num_bytes));
	N->m_over = *over;
	N->ReceiveData();
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
		m_PendingPlayerJoins.push(PlayerJoinEvent{ p->playerId, p->transform, p->bRunning, p->bAiming, p->fAimPitch });
		break;
	}
	case S2C_REMOVE_PLAYER:
	{
		if (size < static_cast<int>(sizeof(S2C_RemovePlayer))) return;
		auto* p = reinterpret_cast<const S2C_RemovePlayer*>(data);
		m_PendingPlayerLeaves.push(p->playerId);
		break;
	}
	case S2C_SPAWN_ZOMBIE:
	{
		if (size < static_cast<int>(sizeof(S2C_SpawnZombie))) return;
		auto* p = reinterpret_cast<const S2C_SpawnZombie*>(data);
		m_PendingSpawns.push(SpawnEvent{ p->zombieId, Vector3(p->x, p->y, p->z) });
		break;
	}
	case S2C_DESPAWN_ZOMBIE:
	{
		if (size < static_cast<int>(sizeof(S2C_DespawnZombie))) return;
		auto* p = reinterpret_cast<const S2C_DespawnZombie*>(data);
		m_PendingDespawns.push(p->zombieId);
		auto it = m_ZombieStates.find(p->zombieId);
		if (it != m_ZombieStates.end())
			it->second.valid = false;
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
		m_ZombieStates[p->zombieId] = state;
		break;
	}
	case S2C_ZOMBIE_ATTACK:
	{
		if (size < static_cast<int>(sizeof(S2C_ZombieAttack))) return;
		auto* p = reinterpret_cast<const S2C_ZombieAttack*>(data);
		m_PendingAttacks.push(AttackEvent{ p->zombieId, p->targetPlayerId, p->damage });
		break;
	}
	case S2C_SHOOT_RESULT:
	{
		if (size < static_cast<int>(sizeof(S2C_ShootResult))) return;
		auto* p = reinterpret_cast<const S2C_ShootResult*>(data);
		ShootResultEvent ev;
		ev.shooterPlayerId = p->shooterPlayerId;
		ev.bHit            = p->bHit;
		ev.v3HitPoint      = Vector3{ p->hitX, p->hitY, p->hitZ };
		ev.v3HitNormal     = Vector3{ p->hitNormalX, p->hitNormalY, p->hitNormalZ };
		ev.hitZombieId     = p->hitZombieId;
		ev.damage          = p->damage;
		ev.v3MuzzlePos     = Vector3{ p->muzzleX, p->muzzleY, p->muzzleZ };
		ev.v3ShootDir      = Vector3{ p->shootDirX, p->shootDirY, p->shootDirZ };
		m_PendingShootResults.push(ev);
		break;
	}
	case S2C_TRANSFORM: {
		if (size < static_cast<int>(sizeof(S2C_Transform))) return;
		auto* p = reinterpret_cast<const S2C_Transform*>(data);
		m_PendingPlayerTransforms.push(PlayerTransformEvent{ p->playerId, p->transform, p->bRunning, p->bAiming, p->fAimPitch });
		break;
	}
	case S2C_PLAYER_RELOAD: {
		if (size < static_cast<int>(sizeof(S2C_PlayerReload))) return;
		auto* p = reinterpret_cast<const S2C_PlayerReload*>(data);
		std::cout << "[Network] Received S2C_PLAYER_RELOAD for player " << p->playerId << std::endl;
		m_PendingPlayerReloads.push(p->playerId);
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

	// C2S_TRANSFORM으로 위치 전송 (서버가 플레이어 위치를 좀비 AI에 사용)
	C2S_Transform p;
	p.size = sizeof(C2S_Transform);
	p.type = C2S_TRANSFORM;
	memset(&p.transform, 0, sizeof(p.transform));

	// 단위 행렬 + 위치만 설정 (서버는 m[3][0..2]만 사용)
	p.transform.m[0][0] = 1.f;
	p.transform.m[1][1] = 1.f;
	p.transform.m[2][2] = 1.f;
	p.transform.m[3][3] = 1.f;
	p.transform.m[3][0] = m_v3LocalPlayerPos.x;
	p.transform.m[3][1] = m_v3LocalPlayerPos.y;
	p.transform.m[3][2] = m_v3LocalPlayerPos.z;

	SendPacket(&p, p.size);
}

// -----------------------------------------------------------------------
// 좀비 이벤트 소비 (Task 6/7/9)
// -----------------------------------------------------------------------

std::vector<SpawnEvent> NetworkManager::ConsumeSpawnEvents()
{
	std::vector<SpawnEvent> out;
	SpawnEvent ev;
	while (m_PendingSpawns.try_pop(ev))
		out.push_back(ev);
	return out;
}

std::vector<int> NetworkManager::ConsumeDespawnEvents()
{
	std::vector<int> out;
	int id;
	while (m_PendingDespawns.try_pop(id))
		out.push_back(id);
	return out;
}

std::vector<AttackEvent> NetworkManager::ConsumeAttackEvents()
{
	std::vector<AttackEvent> out;
	AttackEvent ev;
	while (m_PendingAttacks.try_pop(ev))
		out.push_back(ev);
	return out;
}

void NetworkManager::SendPlayerShoot(const Vector3& v3Origin, const Vector3& v3Direction, const Vector3& v3MuzzlePos)
{
	if (!m_bConnected || m_bOfflineMode) return;

	C2S_PlayerShoot p;
	p.size    = sizeof(C2S_PlayerShoot);
	p.type    = C2S_PLAYER_SHOOT;
	p.originX = v3Origin.x;
	p.originY = v3Origin.y;
	p.originZ = v3Origin.z;
	p.dirX    = v3Direction.x;
	p.dirY    = v3Direction.y;
	p.dirZ    = v3Direction.z;
	p.muzzleX = v3MuzzlePos.x;
	p.muzzleY = v3MuzzlePos.y;
	p.muzzleZ = v3MuzzlePos.z;
	SendPacket(&p, p.size);
}

std::vector<ShootResultEvent> NetworkManager::ConsumeShootResults()
{
	std::vector<ShootResultEvent> out;
	ShootResultEvent ev;
	while (m_PendingShootResults.try_pop(ev))
		out.push_back(ev);
	return out;
}

void NetworkManager::SendPlayerReload()
{
	if (!m_bConnected || m_bOfflineMode) return;

	C2S_PlayerReload p;
	p.size = sizeof(C2S_PlayerReload);
	p.type = C2S_PLAYER_RELOAD;
	SendPacket(&p, p.size);
}

std::vector<int> NetworkManager::ConsumePlayerReloads()
{
	std::vector<int> out;
	int playerId;
	while (m_PendingPlayerReloads.try_pop(playerId))
		out.push_back(playerId);
	return out;
}

std::vector<PlayerJoinEvent> NetworkManager::ConsumePlayerJoins()
{
	std::vector<PlayerJoinEvent> out;
	PlayerJoinEvent ev;
	while (m_PendingPlayerJoins.try_pop(ev))
		out.push_back(ev);
	return out;
}

std::vector<int> NetworkManager::ConsumePlayerLeaves()
{
	std::vector<int> out;
	int id;
	while (m_PendingPlayerLeaves.try_pop(id))
		out.push_back(id);
	return out;
}

std::vector<PlayerTransformEvent> NetworkManager::ConsumePlayerTransforms()
{
	std::vector<PlayerTransformEvent> out;
	PlayerTransformEvent ev;
	while (m_PendingPlayerTransforms.try_pop(ev))
		out.push_back(ev);
	return out;
}

bool NetworkManager::GetLatestZombieState(int zombieId, ZombieServerState& outState) const
{
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
