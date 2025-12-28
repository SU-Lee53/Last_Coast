#include "pch.h"
#include "NetworkManager.h"

#include "Packets.h"


/////////////////////////////////////////////////////////////////////////////////////////////////////////
// 2025.11.03
// NetworkManager() By 이승욱
// ImGui 를 이용하여 IP 를 입력받고 연결을 시도
//
// 11.19
// Critical Section, Event 초기화

// Static 동기화 객체 초기화
CRITICAL_SECTION NetworkManager::g_hCS{};
HANDLE NetworkManager::g_hPlayerWritePacketEvent = nullptr;
HANDLE NetworkManager::g_hPacketReceivedEvent = nullptr;
HANDLE NetworkManager::g_hNetworkThread = nullptr;

NetworkManager::NetworkManager()
{
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		__debugbreak();
		PostQuitMessage(0);
	}

	m_hClientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (m_hClientSocket == INVALID_SOCKET) {
		__debugbreak();
		PostQuitMessage(0);
	}


	InitializeCriticalSection(&g_hCS);

	// 이벤트는 자동 리셋을 사용
	g_hPlayerWritePacketEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	
	g_hPacketReceivedEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
}

NetworkManager::~NetworkManager()
{
	Disconnect();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// 2025.11.03
// ConnectToServer() By 이승욱
// ImGui 를 이용하여 IP 를 입력받고 연결을 시도
// 
// 2025.11.04
// Local 서버를 이용한 테스트 완료

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
			inet_pton(AF_INET, m_cstrServerIP, &serveraddr.sin_addr);
			serveraddr.sin_port = htons(SERVERPORT);
			int retval = connect(m_hClientSocket, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
			if (retval == SOCKET_ERROR) {
				m_strErrorLog = err_display("socket()");
			}
			else {
				m_bConnected = true;
				g_hNetworkThread = CreateThread(NULL, 0, ProcessNetwork, (LPVOID)m_hClientSocket, 0, NULL);
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

		// 하지 말것
		//if (m_bConnected) {
		//	if (ImGui::Button("Start")) {
		//		SCENE->ChangeScene<TestScene>();
		//	}
		//}

	}
	ImGui::End();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// 2025.11.04
// SendData() By 이승욱
// 서버로 ClientToServerPacket 을 전송
// 전송하기 전에 몇개의 패킷을 보낼지 먼저 알려줘야 함
// return value : recv() 의 retval 을 이용한 성공/실패 여부
// 
// 2025.11.04
// Local 서버를 이용한 테스트 완료
//
// 2025.11.14
// 패킷 변경에 따라 Send 방식 변경
//
// 11.19
// 인자 제거, m_PacketToSend 의 데이터 전송


bool NetworkManager::SendData()
{
	if (!m_bConnected) 
		return false;

	assert(m_bConnected);
	int nBytesToSend = sizeof(ClientToServerPacket);
	int retval{};
	int nSumOfRetval{};

	retval += send(m_hClientSocket, (char*)&m_PacketToSend, sizeof(ClientToServerPacket), 0);
	if (retval == SOCKET_ERROR) {
		m_strErrorLog = err_display("send()");
	}
	else {
		nSumOfRetval += retval;
	}

	//ImGui::Text(m_strErrorLog.c_str());		// TODO : 확인 후 지울것

	return nSumOfRetval == nBytesToSend;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// 2025.11.06
// Disconnect() By 민정원
// 연결해제

void NetworkManager::Disconnect()
{
	m_bConnected = false;
	CloseHandle(g_hNetworkThread);
	closesocket(m_hClientSocket);
	DeleteCriticalSection(&g_hCS);
	WSACleanup();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// 2025.11.04
// ReceiveData By 이승욱
// 테스트용 임시
//
// 11.19
// 인자 제거, m_PacketReceived 로 데이터 받아옴

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// 2025.11.27
// ReceiveData By 민정원
// Rock 데이터 받는코드 추가

bool NetworkManager::ReceiveData()
{
	if (!m_bConnected) 
		return false;
	int retval = 0;
	retval = recv(m_hClientSocket, (char*)&m_PacketReceived, sizeof(ServertoClientPlayerPacket), MSG_WAITALL);
	if (retval == SOCKET_ERROR) {
		err_quit("recv()");
	}
	retval += recv(m_hClientSocket, (char*)&m_PacketRocksReceived, sizeof(ServertoClientRockPacket), MSG_WAITALL);
	SetEvent(g_hPacketReceivedEvent);  // 이벤트 시그널

	return retval == sizeof(ServertoClientPlayerPacket) + sizeof(ServertoClientRockPacket);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// 2025.11.04
// WritePacketData By 이승욱
// Player::MakePacketToSend() 로 만들어진 패킷을 NetworkManager::m_PacketToSend 로 복사

void NetworkManager::WritePacketData(const ClientToServerPacket& packet)
{
	::memcpy(&m_PacketToSend, &packet, sizeof(ClientToServerPacket));
	SetEvent(g_hPlayerWritePacketEvent);  // 이벤트 시그널
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// 2025.11.04
// GetReceivedPacketData By 이승욱
// 서버에서 가져온 패킷을 리턴
// 그전에 NetworkManager 가 서버에서 패킷을 받아와야하므로 이벤트로 동기화함

ServertoClientPlayerPacket NetworkManager::GetReceivedPacketData() const
{
	WaitForSingleObject(g_hPacketReceivedEvent, INFINITE);
	return m_PacketReceived;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// 2025.11.04
// GetReceivedPacketData By 이승욱
// 서버에서 가져온 패킷을 리턴
// 그전에 NetworkManager 가 서버에서 패킷을 받아와야하므로 이벤트로 동기화함

ServertoClientRockPacket NetworkManager::GetReceivedRockPacketData()
{
	return m_PacketRocksReceived;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////
// 2025.11.16
// ProcessNetwork By 이승욱
// 멀티스레드

DWORD WINAPI NetworkManager::ProcessNetwork(LPVOID arg)
{
	// 1. 연결 -> Main 스레드에서
	//while (!NETWORK->m_bConnected) {
	//	NETWORK->ConnectToServer();
	//}
	
	// 2. 시작 대기
	// TODO : 신호를 받아 Scene 을 변경하려면 새로운 방법이 필요할 수도 있음
	StartPacket startPacket{};
	while (true) {
		int retval = recv(NETWORK->m_hClientSocket, (char*)&startPacket, sizeof(StartPacket), MSG_WAITALL);
		NETWORK->m_nPlayerID = startPacket.id;
		if (startPacket.startFlag) {
			NETWORK->m_bGameBegin = true;
			NETWORK->m_bOfflineMode = false;
			break;
		}
	}

	// 3. 패킷 송,수신
	while (NETWORK->m_bConnected) {
		WaitForSingleObject(g_hPlayerWritePacketEvent, INFINITE);
		NETWORK->SendData();
		NETWORK->ReceiveData();
	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// 2025.11.11
// MakePacketToSend() By 민정원
// 서버로 보낼 패킷 생성.


/////////////////////////////////////////////////////////////////////////////////////////////////////////
// 2025.11.14
// MakePacketToSend() By 민정원
// player에서 다시 구현.

//void NetworkManager::MakePacketToSend(ClientToServerPacket& packet , std::shared_ptr<Player> Player)
//{
//	packet.id = 999;
//	packet.transformData.mtxPlayerTransform = Player->GetTransform().GetWorldMatrix();
//	packet.shotData.v3RayDirection = static_pointer_cast<SpaceshipPlayer>(Player)->GetRayDirection();
//	packet.shotData.v3RayPosition = static_pointer_cast<SpaceshipPlayer>(Player)->GetRayPos();
//}
