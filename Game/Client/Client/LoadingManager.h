#pragma once

struct LoadingTicket {
	uint64 id = 0;

	bool IsValid() const {
		return id != 0;
	}
};

class LoadingManager {
	DECLARE_SINGLE(LoadingManager)

	~LoadingManager();

	void Initialize();
	void Shutdown();

	LoadingTicket Enqueue(std::function<void()> job);

	bool IsComplete(LoadingTicket ticket) const;
	bool IsRunning(LoadingTicket ticket) const;
	bool IsIdle() const;

	bool HasFailed(LoadingTicket ticket) const;
	bool HasError() const;
	std::string GetLastError() const;

private:
	struct LoadingJob {
		uint64 ticket = 0;
		std::function<void()> fn;
	};

	void WorkerLoop();

private:
	std::thread m_WorkerThread;
	mutable std::mutex m_mtx;
	std::condition_variable m_CV;
	std::queue<LoadingJob> m_JobQueue;

	std::atomic_bool m_bInitialized = false;
	std::atomic_bool m_bStopRequested = false;

	std::atomic_uint64_t m_un64NextTicket = 1;
	std::atomic_uint64_t m_un64SubmittedTicket = 0;
	std::atomic_uint64_t m_un64RunningTicket = 0;
	std::atomic_uint64_t m_un64CompletedTicket = 0;
	std::atomic_uint64_t m_un64FailedTicket = 0;

	mutable std::mutex m_mtxError;
	std::string m_strLastError;

};

