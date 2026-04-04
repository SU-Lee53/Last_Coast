#include "pch.h"
#include "ThreadManager.h"
#include "Service.h"
#include "Session.h"
#include <chrono>

using namespace std::chrono_literals;

char sendBuffer[] = "Hello World!";

class ServerSession : public Session
{
public:
	~ServerSession()
	{
		cout << "~ServerSession" << endl;
	}

	virtual void OnConnected() override
	{
		std::cout << "Connected To Server" << std::endl;
		Send((BYTE*)sendBuffer, sizeof(sendBuffer));
	}

	virtual int32 OnRecv(BYTE* buffer, int32 len) override
	{
		//	Echo
		std::cout << "OnRecv Len = " << len << std::endl;
		std::this_thread::sleep_for(1s);
		Send((BYTE*)sendBuffer, sizeof(sendBuffer));
		return len;
	}

	virtual void OnSend(int32 len) override
	{
		std::cout << "OnSend Len = " << len << std::endl;
	}

	virtual void OnDisconnected() override
	{
		std::cout << "Disconnected" << std::endl;
	}
};

int main()
{
	std::this_thread::sleep_for(1s);

	ClientServiceRef service = std::make_shared<ClientService>(NetAddress(L"127.0.0.1", 9000), std::make_shared<IocpCore>(), std::make_shared<ServerSession>, 1);

	ASSERT_CRASH(service->Start());

	for (int32 i = 0; i < 2; ++i)
	{
		GThreadManager->Launch([=]()
			{
				while (true)
				{
					service->GetIocpCore()->Dispatch();
				}
			});
	}

	GThreadManager->Join();
}
