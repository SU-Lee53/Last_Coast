#pragma once

class Session;

enum class EventType : uint8
{
	Connect,
	Disconnect,
	Accept,
	Recv,
	Send
};

//	offset 0번에는 OVERLAPPED
//	IocpEvent 포인터를 사용하는게 OVERLAPPED를 사용하는거랑 똑같다.
class IocpEvent : public OVERLAPPED
{
public:
	IocpEvent(EventType type);

	void		Init();

public:
	EventType	eventType;
	IocpObjectRef owner;
};

class ConnectEvent : public IocpEvent
{
public:
	ConnectEvent() : IocpEvent(EventType::Connect) {}
};

class DisconnectEvent : public IocpEvent
{
public:
	DisconnectEvent() : IocpEvent(EventType::Disconnect) {}
};

class AcceptEvent : public IocpEvent
{
public:
	AcceptEvent() : IocpEvent(EventType::Accept) {}	// AcceptEvent가 생성될때 부모클래스인 IocpEvent 생성자를 호출한다.

public:
	SessionRef session = nullptr;
};

class RecvEvent : public IocpEvent
{
public:
	RecvEvent() : IocpEvent(EventType::Recv) {}
};

class SendEvent : public IocpEvent
{
public:
	SendEvent() : IocpEvent(EventType::Send) {}

	std::vector<BYTE> buffer;
};
