#include "pch.h"
#include "NetworkManager.h"
#include "Packets.h"
#include "TextBox.h"

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
				/*SendLoginPacket();*/
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
					// SendLoginPacket(); // 제거: 명시적 로그인 대기
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

void NetworkManager::SendLogin(const std::string& id, const std::string& pw)
{
	if (m_hClientSocket == INVALID_SOCKET || !m_bConnected)
		return;

	m_nLoginState = 0; // 진행 중
	C2S_Login packet;
	packet.size = sizeof(C2S_Login);
	packet.type = C2S_LOGIN;
	strncpy_s(packet.username, id.c_str(), sizeof(packet.username));
	strncpy_s(packet.password, pw.c_str(), sizeof(packet.password));

	SendPacket(&packet, packet.size);
}

void NetworkManager::SendRegister(const std::string& id, const std::string& pw)
{
	if (m_hClientSocket == INVALID_SOCKET || !m_bConnected)
		return;

	m_nRegisterState = 0; // 진행 중
	C2S_Register packet;
	packet.size = sizeof(C2S_Register);
	packet.type = C2S_REGISTER;
	strncpy_s(packet.username, id.c_str(), sizeof(packet.username));
	strncpy_s(packet.password, pw.c_str(), sizeof(packet.password));

	SendPacket(&packet, packet.size);
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
		if (p->success) {
			m_nLoginState = 1;
			m_bGameBegin = true;
		} else {
			m_nLoginState = -1;
		}
		break;
	}
	case S2C_REGISTER_RESULT:
	{
		if (size < static_cast<int>(sizeof(S2C_RegisterResult))) return;
		auto* p = reinterpret_cast<const S2C_RegisterResult*>(data);
		if (p->success) {
			m_nRegisterState = 1;
		} else {
			m_nRegisterState = -1;
		}
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
		
		char nameBuf[MAX_NAME_LEN];
		memcpy_s(nameBuf, sizeof(nameBuf), p->username, MAX_NAME_LEN);
		nameBuf[MAX_NAME_LEN - 1] = '\0';

		PlayerJoinEvent joinEv{ p->playerId, p->transform, p->bRunning, p->bAiming, p->fAimPitch, p->weaponType, nameBuf, p->bReady, p->characterType };
		m_PendingPlayerJoins.push(joinEv);
		{
			std::lock_guard<std::mutex> lk(m_RoomPlayersMutex);
			m_RoomPlayers[p->playerId] = joinEv; // 큐와 별개로 방 멤버 보존
		}
		break;
	}
	case S2C_REMOVE_PLAYER:
	{
		if (size < static_cast<int>(sizeof(S2C_RemovePlayer))) return;
		auto* p = reinterpret_cast<const S2C_RemovePlayer*>(data);
		m_PendingPlayerLeaves.push(p->playerId);
		{
			std::lock_guard<std::mutex> lk(m_RoomPlayersMutex);
			m_RoomPlayers.erase(p->playerId);
		}
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
	case S2C_ZOMBIE_STATE_BATCH:
	{
		if (size < static_cast<int>(offsetof(S2C_ZombieStateBatch, entries))) return;
		auto* p = reinterpret_cast<const S2C_ZombieStateBatch*>(data);
		int count = p->count;
		if (count > MAX_ZOMBIE_BATCH) count = MAX_ZOMBIE_BATCH;
		const double dRecv = GetNetTimeSec();
		for (int i = 0; i < count; ++i)
		{
			const ZombieStateEntry& e = p->entries[i];
			ZombieServerState state;
			state.x             = e.x;
			state.z             = e.z;
			state.yaw           = e.yaw;
			state.waypointX     = e.waypointX;
			state.waypointZ     = e.waypointZ;
			state.behaviorState = e.behaviorState;
			state.receivedTime  = dRecv;
			state.valid         = true;
			m_ZombieStates[e.zombieId] = state;
		}
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
		m_PendingPlayerTransforms.push(PlayerTransformEvent{ p->playerId, p->transform, p->bRunning, p->bAiming, p->fAimPitch, GetNetTimeSec() });
		break;
	}
	case S2C_PLAYER_RELOAD: {
		if (size < static_cast<int>(sizeof(S2C_PlayerReload))) return;
		auto* p = reinterpret_cast<const S2C_PlayerReload*>(data);
		std::cout << "[Network] Received S2C_PLAYER_RELOAD for player " << p->playerId << std::endl;
		m_PendingPlayerReloads.push(p->playerId);
		break;
	}
	case S2C_PLAYER_MELEE: {
		if (size < static_cast<int>(sizeof(S2C_PlayerMelee))) return;
		auto* p = reinterpret_cast<const S2C_PlayerMelee*>(data);
		m_PendingPlayerMelees.push(p->attackerPlayerId);
		break;
	}
	case S2C_MELEE_HIT: {
		if (size < static_cast<int>(sizeof(S2C_MeleeHit))) return;
		auto* p = reinterpret_cast<const S2C_MeleeHit*>(data);
		MeleeHitEvent ev;
		ev.attackerPlayerId = p->attackerPlayerId;
		ev.zombieId         = p->zombieId;
		ev.damage           = p->damage;
		ev.v3HitPoint       = Vector3{ p->hitX, p->hitY, p->hitZ };
		m_PendingMeleeHits.push(ev);
		break;
	}
	case S2C_CHAT: {
		if (size < static_cast<int>(sizeof(S2C_Chat))) return;
		auto* p = reinterpret_cast<const S2C_Chat*>(data);
		ChatMessageEvent ev;
		ev.playerId = p->playerId;
		char name[MAX_NAME_LEN]; memcpy_s(name, sizeof(name), p->username, MAX_NAME_LEN); name[MAX_NAME_LEN - 1] = '\0';
		char msg[MAX_CHAT_LEN];  memcpy_s(msg, sizeof(msg), p->message, MAX_CHAT_LEN);   msg[MAX_CHAT_LEN - 1] = '\0';
		ev.username = name;
		ev.message  = msg;
		m_PendingChatMessages.push(ev);
		break;
	}
	case S2C_PLAYER_WEAPON: {
		if (size < static_cast<int>(sizeof(S2C_PlayerWeapon))) return;
		auto* p = reinterpret_cast<const S2C_PlayerWeapon*>(data);
		m_PendingPlayerWeapons.push(WeaponChangeEvent{ p->playerId, p->weaponType });
		break;
	}
	case S2C_PLAYER_CHARACTER: {
		if (size < static_cast<int>(sizeof(S2C_PlayerCharacter))) return;
		auto* p = reinterpret_cast<const S2C_PlayerCharacter*>(data);
		m_PendingPlayerCharacters.push(CharacterChangeEvent{ p->playerId, p->characterType });
		break;
	}
	case S2C_GAME_EVENT: {
		if (size < static_cast<int>(sizeof(S2C_GameEvent))) return;
		auto* p = reinterpret_cast<const S2C_GameEvent*>(data);
		m_PendingGameEvents.push(GameEventMsg{ p->eventId, Vector3{ p->x, p->y, p->z }, p->fTargetValue, p->fDuration, p->presetId });
		break;
	}
	case S2C_READY_STATE: {
		if (size < static_cast<int>(sizeof(S2C_ReadyState))) return;
		auto* p = reinterpret_cast<const S2C_ReadyState*>(data);
		m_PendingReadyStates.push(ReadyStateEvent{ p->playerId, p->bReady });
		break;
	}
	case S2C_GAME_START: {
		m_bPendingGameStart.store(true);
		break;
	}
	case S2C_GAME_BEGIN: {
		m_bPendingGameBegin.store(true);
		break;
	}
	case S2C_HOST_CHANGE: {
		if (size < static_cast<int>(sizeof(S2C_HostChange))) return;
		auto* p = reinterpret_cast<const S2C_HostChange*>(data);
		m_nHostId.store(p->hostPlayerId);
		break;
	}
	case S2C_ESCAPE_STATE: {
		if (size < static_cast<int>(sizeof(S2C_EscapeState))) return;
		auto* p = reinterpret_cast<const S2C_EscapeState*>(data);
		m_nEscapePhase.store(static_cast<int>(p->phase));
		m_fEscapeRemain.store(p->fRemainSeconds);
		break;
	}
	case S2C_GAME_END: {
		m_bPendingGameEnd.store(true);
		break;
	}
	case S2C_PLAYER_DEATH: {
		if (size < static_cast<int>(sizeof(S2C_PlayerDeath))) return;
		auto* p = reinterpret_cast<const S2C_PlayerDeath*>(data);
		m_PendingPlayerDeaths.push(PlayerDeathEvent{ p->playerId, p->fRespawnSeconds });
		break;
	}
	case S2C_PLAYER_RESPAWN: {
		if (size < static_cast<int>(sizeof(S2C_PlayerRespawn))) return;
		auto* p = reinterpret_cast<const S2C_PlayerRespawn*>(data);
		m_PendingPlayerRespawns.push(PlayerRespawnEvent{ p->playerId, Vector3{ p->x, p->y, p->z } });
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

void NetworkManager::SendPlayerShoot(const Vector3& v3Origin, const Vector3& v3Direction, const Vector3& v3MuzzlePos, float damage)
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
	p.damage  = damage;
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

void NetworkManager::SendPlayerWeapon(unsigned char weaponType)
{
	if (!m_bConnected || m_bOfflineMode) return;

	C2S_PlayerWeapon p;
	p.size = sizeof(C2S_PlayerWeapon);
	p.type = C2S_PLAYER_WEAPON;
	p.weaponType = weaponType;
	SendPacket(&p, p.size);
}

std::vector<WeaponChangeEvent> NetworkManager::ConsumePlayerWeapons()
{
	std::vector<WeaponChangeEvent> out;
	WeaponChangeEvent ev;
	while (m_PendingPlayerWeapons.try_pop(ev))
		out.push_back(ev);
	return out;
}

void NetworkManager::SendPlayerCharacter(unsigned char characterType)
{
	if (!m_bConnected || m_bOfflineMode) return;

	C2S_PlayerCharacter p;
	p.size = sizeof(C2S_PlayerCharacter);
	p.type = C2S_PLAYER_CHARACTER;
	p.characterType = characterType;
	SendPacket(&p, p.size);
}

std::vector<CharacterChangeEvent> NetworkManager::ConsumePlayerCharacters()
{
	std::vector<CharacterChangeEvent> out;
	CharacterChangeEvent ev;
	while (m_PendingPlayerCharacters.try_pop(ev))
		out.push_back(ev);
	return out;
}

void NetworkManager::SendChat(const std::string& message)
{
	if (!m_bConnected || m_bOfflineMode) return;
	if (message.empty()) return;

	C2S_Chat p;
	p.size = sizeof(C2S_Chat);
	p.type = C2S_CHAT;
	strncpy_s(p.message, message.c_str(), MAX_CHAT_LEN - 1);
	SendPacket(&p, p.size);
}

std::vector<ChatMessageEvent> NetworkManager::ConsumeChatMessages()
{
	std::vector<ChatMessageEvent> out;
	ChatMessageEvent ev;
	while (m_PendingChatMessages.try_pop(ev))
		out.push_back(ev);
	return out;
}

std::vector<GameEventMsg> NetworkManager::ConsumeGameEvents()
{
	std::vector<GameEventMsg> out;
	GameEventMsg ev;
	while (m_PendingGameEvents.try_pop(ev))
		out.push_back(ev);
	return out;
}

// ── 탈출 시퀀스 (서버 권위) ──────────────────────────────────────────────────
bool NetworkManager::GetEscapeState(unsigned char& outPhase, float& outRemainSeconds) const
{
	const int nPhase = m_nEscapePhase.load();
	if (nPhase < 0) return false; // 아직 서버 상태 수신 전
	outPhase = static_cast<unsigned char>(nPhase);
	outRemainSeconds = m_fEscapeRemain.load();
	return true;
}

void NetworkManager::SendPlayerEscape()
{
	if (!m_bConnected || m_bOfflineMode) return;

	C2S_PlayerEscape p;
	p.size = sizeof(C2S_PlayerEscape);
	p.type = C2S_PLAYER_ESCAPE;
	SendPacket(&p, p.size);
}

bool NetworkManager::ConsumeGameEnd()
{
	return m_bPendingGameEnd.exchange(false);
}

void NetworkManager::SendReady(bool bReady)
{
	if (!m_bConnected || m_bOfflineMode) return;

	C2S_Ready p;
	p.size   = sizeof(C2S_Ready);
	p.type   = C2S_READY;
	p.bReady = bReady;
	SendPacket(&p, p.size);
}

void NetworkManager::SendGameStart()
{
	if (!m_bConnected || m_bOfflineMode) return;

	C2S_GameStart p;
	p.size = sizeof(C2S_GameStart);
	p.type = C2S_GAME_START;
	SendPacket(&p, p.size);
}

std::vector<ReadyStateEvent> NetworkManager::ConsumeReadyStates()
{
	std::vector<ReadyStateEvent> out;
	ReadyStateEvent ev;
	while (m_PendingReadyStates.try_pop(ev))
		out.push_back(ev);
	return out;
}

bool NetworkManager::ConsumeGameStart()
{
	return m_bPendingGameStart.exchange(false);
}

void NetworkManager::SendLoadComplete()
{
	if (!m_bConnected || m_bOfflineMode) return;

	C2S_LoadComplete p;
	p.size = sizeof(C2S_LoadComplete);
	p.type = C2S_LOAD_COMPLETE;
	SendPacket(&p, p.size);
}

bool NetworkManager::ConsumeGameBegin()
{
	return m_bPendingGameBegin.exchange(false);
}

void NetworkManager::SendPlayerMelee(const Vector3& v3Origin, const Vector3& v3Direction)
{
	if (!m_bConnected || m_bOfflineMode) return;

	C2S_PlayerMelee p;
	p.size    = sizeof(C2S_PlayerMelee);
	p.type    = C2S_PLAYER_MELEE;
	p.originX = v3Origin.x;
	p.originY = v3Origin.y;
	p.originZ = v3Origin.z;
	p.dirX    = v3Direction.x;
	p.dirY    = v3Direction.y;
	p.dirZ    = v3Direction.z;
	SendPacket(&p, p.size);
}

std::vector<int> NetworkManager::ConsumePlayerMelees()
{
	std::vector<int> out;
	int attackerId;
	while (m_PendingPlayerMelees.try_pop(attackerId))
		out.push_back(attackerId);
	return out;
}

std::vector<MeleeHitEvent> NetworkManager::ConsumeMeleeHits()
{
	std::vector<MeleeHitEvent> out;
	MeleeHitEvent ev;
	while (m_PendingMeleeHits.try_pop(ev))
		out.push_back(ev);
	return out;
}

std::vector<PlayerDeathEvent> NetworkManager::ConsumePlayerDeaths()
{
	std::vector<PlayerDeathEvent> out;
	PlayerDeathEvent ev;
	while (m_PendingPlayerDeaths.try_pop(ev))
		out.push_back(ev);
	return out;
}

std::vector<PlayerRespawnEvent> NetworkManager::ConsumePlayerRespawns()
{
	std::vector<PlayerRespawnEvent> out;
	PlayerRespawnEvent ev;
	while (m_PendingPlayerRespawns.try_pop(ev))
		out.push_back(ev);
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

std::vector<PlayerJoinEvent> NetworkManager::GetRoomPlayersSnapshot()
{
	std::vector<PlayerJoinEvent> out;
	std::lock_guard<std::mutex> lk(m_RoomPlayersMutex);
	out.reserve(m_RoomPlayers.size());
	for (auto& [id, ev] : m_RoomPlayers)
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
	// GetTickCount64는 해상도가 ~15.6ms — 스냅샷 시각/렌더 시각이 계단식으로
	// 양자화되어 보간 위치가 멈췄다-점프를 반복한다 → QPC(µs급)로 교체
	static const LARGE_INTEGER Freq = [] { LARGE_INTEGER f; QueryPerformanceFrequency(&f); return f; }();
	static const LARGE_INTEGER Epoch = [] { LARGE_INTEGER c; QueryPerformanceCounter(&c); return c; }();
	LARGE_INTEGER now;
	QueryPerformanceCounter(&now);
	return static_cast<float>(
		static_cast<double>(now.QuadPart - Epoch.QuadPart) / static_cast<double>(Freq.QuadPart));
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
