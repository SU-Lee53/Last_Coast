#include "pch.h"
#include "Session.h"
#include "SocketUtils.h"
#include "Service.h"

Session::Session()
{
	_socket = SocketUtils::CreateSocket();
}

Session::~Session()
{
	SocketUtils::Close(_socket);
}

void Session::Send(BYTE* buffer, int32 len)
{
	//	생각할 문제
	//	1. 버퍼 관리
	//	2. sendEvent 관리? 단일? 여러개? WSASend 중첩?
	//

	//	Temp
	SendEvent* sendEvent = xnew<SendEvent>();
	sendEvent->owner = shared_from_this();
	sendEvent->buffer.resize(len);
	memcpy(sendEvent->buffer.data(), buffer, len);

	WRITE_LOCK;
	RegisterSend(sendEvent);
}

bool Session::Connect()		//	서버끼리 서로 연결할 때 필요
{
	return RegisterConnect();
}

void Session::Disconnect(const WCHAR* cause)
{
	if (_connected.exchange(false) == false)
		return;

	std::wcout << "Disconnect : " << cause << std::endl;

	OnDisconnected();	//	컨텐츠 코드에서 재정의
	SocketUtils::Close(_socket);
	GetService()->ReleaseSession(GetSessionRef());

	RegisterDisconnect();
}

HANDLE Session::GetHandle()
{
	//	reinterpret_cast는 비트 그대로 해석
	//	원래라면 cast하려는 타입으로 해석하겠다는 의미
	//	그러나 여기서 사용하는 _socket의 타입은 SOCKET
	//	SOCKET은 UINT_PTR 그저 정수 타입 (포인터 크기 정수) -> 64bit면 8byte
	//	return 타입이 HANDLE이므로
	//	HANDLE로 타입만 변경한 의미

	return reinterpret_cast<HANDLE>(_socket);
}

void Session::Dispatch(IocpEvent* iocpEvent, int32 numOfBytes)
{
	switch (iocpEvent->eventType)
	{
	case EventType::Connect:
		ProcessConnect();
		break;
	case EventType::Disconnect:
		ProcessDisconnect();
		break;
	case EventType::Recv:
		ProcessRecv(numOfBytes);
		break;
	case EventType::Send:
		ProcessSend(static_cast<SendEvent*>(iocpEvent), numOfBytes);
		break;
	default:
		break;
	}
}

bool Session::RegisterConnect()
{
	if (IsConnected())
		return false;

	if (GetService()->GetServiceType() != ServiceType::Client)
		return false;

	if (SocketUtils::SetReuseAddress(_socket, true) == false)
		return false;

	if (SocketUtils::BindAnyAddress(_socket, 0) == false)
		return false;

	_connectEvent.Init();
	_connectEvent.owner = shared_from_this();

	DWORD numOfBytes = 0;
	SOCKADDR_IN sockAddr = GetService()->GetNetAddress().GetSockAddr();
	if (false == SocketUtils::ConnectEx(_socket, reinterpret_cast<SOCKADDR*>(&sockAddr), sizeof(sockAddr), nullptr, 0, &numOfBytes, &_connectEvent))
	{
		int32 errorCode = WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			_connectEvent.owner = nullptr;
			return false;
		}
	}

	return true;
}

bool Session::RegisterDisconnect()
{
	_disconnectEvent.Init();
	_disconnectEvent.owner = shared_from_this();

	if (false == SocketUtils::DisconnectEx(_socket, &_disconnectEvent, TF_REUSE_SOCKET, 0))
	{
		int32 errorCode = WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			_disconnectEvent.owner = nullptr;
			return false;
		}
	}

	return true;
}

void Session::RegisterRecv()
{
	if (IsConnected() == false)
		return;

	_recvEvent.Init();						//	초기화
	_recvEvent.owner = shared_from_this();	//	Reference를 늘려서 없어지지 않게 해주기 위함

	WSABUF wsaBuf;
	wsaBuf.buf = reinterpret_cast<char*>(_recvBuffer);
	wsaBuf.len = len32(_recvBuffer);

	DWORD numOfBytes = 0;
	DWORD flags = 0;

	if (SOCKET_ERROR == WSARecv(_socket, &wsaBuf, 1, OUT & numOfBytes, OUT & flags, &_recvEvent, nullptr))
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			HandleError(errorCode);
			_recvEvent.owner = nullptr;	//	Reference 해제
		}
	}
}

void Session::RegisterSend(SendEvent* sendEvent)
{
	if (IsConnected() == false)
		return;

	WSABUF wsaBuf;
	wsaBuf.buf = (char*)sendEvent->buffer.data();
	wsaBuf.len = (ULONG)sendEvent->buffer.size();

	DWORD numOfBytes = 0;
	if (SOCKET_ERROR == WSASend(_socket, &wsaBuf, 1, OUT & numOfBytes, 0, sendEvent, nullptr))
	{
		int32 errorCode = WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			HandleError(errorCode);
			sendEvent->owner = nullptr;
			xdelete(sendEvent);
		}
	}
}

void Session::ProcessConnect()
{
	_connectEvent.owner = nullptr;

	_connected.store(true);

	//	세션 등록
	GetService()->AddSession(GetSessionRef());

	//	컨텐츠 코드에서 재정의
	OnConnected();

	//	수신 등록
	RegisterRecv();

}

void Session::ProcessDisconnect()
{
	_disconnectEvent.owner = nullptr;
}

void Session::ProcessRecv(int32 numOfBytes)
{
	//	RegisterRecv에 걸려있는 Recv가 없으니까
	_recvEvent.owner = nullptr;	//	Reference 해제

	if (numOfBytes == 0)		//	연결이 끊겼다
	{
		Disconnect(L"Recv 0");
	}

	//	TODO
	//	컨텐츠 코드에서 재정의
	OnRecv(_recvBuffer, numOfBytes);

	//	수신 등록 ( 할 일을 다 처리했으니까 다시 수신 등록 )
	RegisterRecv();
}

void Session::ProcessSend(SendEvent* sendEvent, int32 numOfBytes)
{
	sendEvent->owner = nullptr;
	xdelete(sendEvent);

	if (numOfBytes == 0)
	{
		Disconnect(L"Send 0");
		return;
	}

	//	컨텐츠 코드에서 재정의
	OnSend(numOfBytes);
}

void Session::HandleError(int32 errorCode)
{
	switch (errorCode)
	{
	case WSAECONNRESET:
	case WSAECONNABORTED:
		Disconnect(L"HandleError");
		break;
	default:
		std::cout << "Handle Error : " << errorCode << std::endl;
		break;
	}
}
