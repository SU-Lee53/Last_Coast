#pragma once
#include <bit>

constexpr static uint32_t MAX_CMDLIST_POOL_SIZE = 64;

enum class COMMAND_LIST_STATE : uint16 {
	FREE,
	RECORDING,
	SUBMITTED,
	RECLAIMING
};

struct CommandListPair {
	friend class CommandListPool;

	ComPtr<ID3D12GraphicsCommandList> pd3dCommandList;
	ComPtr<ID3D12CommandAllocator> pd3dCommandAllocator;
	uint64 ui64FenceValue = std::numeric_limits<uint64>::max();
	bool bInUse = false;


	HRESULT Close() {
		return pd3dCommandList->Close();
	}

	HRESULT Reset() {
		HRESULT hr{};
		hr = pd3dCommandAllocator->Reset();
		if (FAILED(hr)) {
			return hr;
		}
		hr = pd3dCommandList->Reset(pd3dCommandAllocator.Get(), nullptr);
		return hr;
	}

	void AddPendingResource(ComPtr<ID3D12Resource> pd3dResource) {
		assert(eState.load() == COMMAND_LIST_STATE::RECORDING && "Wrong state");

		if (pd3dResource) {
			m_PendingResources.push_back(pd3dResource);
		}
	}

private:
	uint32_t unPoolIndex = 0;
	std::atomic<COMMAND_LIST_STATE> eState = COMMAND_LIST_STATE::FREE;
	std::vector<ComPtr<ID3D12Resource>> m_PendingResources;

};

class CommandListPool {
public:
	void Initialize(ComPtr<ID3D12Device> pd3dDevice);
	void Shutdown();
	CommandListPair* Allocate(uint64 ui64CompletedFenceValue);
	uint64 ReclaimEnded(ComPtr<ID3D12Fence> pd3dFence);
	uint64 ReclaimEnded(uint64 ui64CompletedFenceValue);

	bool MarkSubmitted(CommandListPair& cmdPair, uint64 un64FenceValue);

public:
	BOOL HasFree();

private:
	static constexpr uint64 MakeFullMask() {
		return ~uint64{ 0 } >> (64 - MAX_CMDLIST_POOL_SIZE);
	}

	void ReturnToFree(UINT unPoolIndex);

private:
	static_assert(MAX_CMDLIST_POOL_SIZE > 0);
	static_assert(MAX_CMDLIST_POOL_SIZE <= 64);

	std::array<CommandListPair, MAX_CMDLIST_POOL_SIZE> m_CmdListPool;
	std::atomic_uint64_t m_un64FreeMask{ 0 };	// 1 if free, 0 if in use

	//std::deque<uint32_t> m_FreeCmdListID{};
	//std::deque<uint32_t> m_InUseCmdListID{};
};

