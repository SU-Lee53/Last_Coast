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

void Session::Disconnect(const WCHAR* cause)
{
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
	case EventType::Recv:
		ProcessRecv(numOfBytes);
		break;
	case EventType::Send:
		ProcessSend(numOfBytes);
		break;
	default:
		break;
	}
}

void Session::RegisterConnect()
{
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

	if (SOCKET_ERROR == WSARecv(_socket, &wsaBuf, 1, OUT &numOfBytes, OUT &flags, &_recvEvent, nullptr))
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			HandleError(errorCode);
			_recvEvent.owner = nullptr;	//	Reference 해제
		}
	}
}

void Session::RegisterSend()
{
}

void Session::ProcessConnect()
{
	_connected.store(true);

	//	세션 등록
	GetService()->AddSession(GetSessionRef());

	//	컨텐츠 코드에서 오버로딩
	OnConnected();

	//	수신 등록
	RegisterRecv();

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
	std::cout << "Recv Data Len = " << numOfBytes << std::endl;

	//	수신 등록 ( 할 일을 다 처리했으니까 다시 수신 등록 )
	RegisterRecv();
}

void Session::ProcessSend(int32 numOfBytes)
{
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
