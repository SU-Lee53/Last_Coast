#include "pch.h"
#include <iostream>

#include "ThreadManager.h"

#include "Service.h"
#include "Session.h"

class GameSession : public Session
{
public:
	virtual int32 OnRecv(BYTE* buffer, int32 len) override
	{
		//	Echo
		std::cout << "OnRecv Len = " << len << std::endl;
		Send(buffer, len);
		return len;
	}

	virtual void OnSend(int32 len) override
	{
		std::cout << "OnSend Len = " << len << std::endl;
	}
};

int main()
{
	ServerServiceRef service = std::make_shared<ServerService>(NetAddress(L"127.0.0.1", 9000), std::make_shared<IocpCore>(), std::make_shared<GameSession>, 100);


	ASSERT_CRASH(service->Start());

	for (int32 i = 0; i < 5; ++i)
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
