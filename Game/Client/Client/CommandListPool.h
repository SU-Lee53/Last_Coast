#pragma once

constexpr static UINT MAX_CMDLIST_POOL_SIZE = 50;

struct CommandListPair {
	friend class CommandListPool;

	ComPtr<ID3D12GraphicsCommandList> pd3dCommandList;
	ComPtr<ID3D12CommandAllocator> pd3dCommandAllocator;
	UINT64 ui64FenceValue = std::numeric_limits<UINT64>::max();
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

};

class CommandListPool {
public:
	void Initialize(ComPtr<ID3D12Device> pd3dDevice);
	CommandListPair* Allocate(UINT64 ui64CompletedFenceValue);
	UINT64 ReclaimEnded(ComPtr<ID3D12Fence> pd3dFence);
	UINT64 ReclaimEnded(UINT64 ui64CompletedFenceValue);

public:
	BOOL HasFree();

private:
	std::array<CommandListPair, MAX_CMDLIST_POOL_SIZE> m_CmdListPool;
	
	std::deque<UINT> m_FreeCmdListID{};
	std::deque<UINT> m_InUseCmdListID{};
};

