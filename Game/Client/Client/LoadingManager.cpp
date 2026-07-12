#include "pch.h"
#include "LoadingManager.h"

LoadingManager::~LoadingManager()
{
	Shutdown();
}

void LoadingManager::Initialize()
{
	if (m_bInitialized.exchange(true) == true) {
		return;
	}

	m_bStopRequested = false;
	m_WorkerThread = std::thread(&LoadingManager::WorkerLoop, this);
}

void LoadingManager::Shutdown()
{
	if (m_bInitialized.exchange(false) == false) {
		return;
	}

	{
		std::lock_guard<std::mutex> lock{ m_mtx };
		m_bStopRequested = true;
	}

	m_CV.notify_all();

	if (m_WorkerThread.joinable()) {
		m_WorkerThread.join();
	}
}

LoadingTicket LoadingManager::Enqueue(std::function<void()> job)
{
	if (!m_bInitialized) {
		Initialize();
	}

	const uint64 un64NewTicket = m_un64NextTicket.fetch_add(1);

	{
		std::lock_guard<std::mutex> lock{ m_mtx };
		m_JobQueue.push(LoadingJob{ 
			.ticket = un64NewTicket,
			.fn = std::move(job)
			}
		);
	}

	m_un64SubmittedTicket.store(un64NewTicket);
	m_CV.notify_one();


	return LoadingTicket{ un64NewTicket };
}

bool LoadingManager::IsComplete(LoadingTicket ticket) const
{
	if (ticket.IsValid() == false) {
		return true;
	}

	return m_un64CompletedTicket.load() >= ticket.id;
}

bool LoadingManager::IsRunning(LoadingTicket ticket) const
{
	return ticket.IsValid() && m_un64RunningTicket.load() == ticket.id;
}

bool LoadingManager::IsIdle() const
{
	return m_un64CompletedTicket.load() >= m_un64SubmittedTicket.load() 
		&& m_un64RunningTicket.load() == 0;
}

bool LoadingManager::HasFailed(LoadingTicket ticket) const
{
	return ticket.IsValid() && m_un64FailedTicket.load() == ticket.id;
}

bool LoadingManager::HasError() const
{
	return m_un64FailedTicket.load() != 0;
}

std::string LoadingManager::GetLastError() const
{
	std::lock_guard<std::mutex> lock{ m_mtxError };
	return m_strLastError;
}

void LoadingManager::WorkerLoop()
{
	while (true) {
		LoadingJob job{};

		{
			std::unique_lock<std::mutex> lock{ m_mtx };
			m_CV.wait(lock, [this]() { 
				return m_bStopRequested || !m_JobQueue.empty(); 
			});

			if (m_bStopRequested && m_JobQueue.empty()) {
				return;
			}

			job = std::move(m_JobQueue.front());
			m_JobQueue.pop();
		}

		m_un64RunningTicket.store(job.ticket);

		try {
			if (job.fn) {
				job.fn();
			}
		}
		catch (const std::exception& e) {
			m_un64FailedTicket.store(job.ticket);
			std::lock_guard<std::mutex> lock{ m_mtxError };
			m_strLastError = e.what();
		}
		catch (...) {
			m_un64FailedTicket.store(job.ticket);
			std::lock_guard<std::mutex> lock{ m_mtxError };
			m_strLastError = "Unknown";
		}

		m_un64CompletedTicket.store(job.ticket);
		m_un64RunningTicket.store(0);
	}


}
