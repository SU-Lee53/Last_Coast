#include "pch.h"
#include "CommandListPool.h"

void CommandListPool::Initialize(ComPtr<ID3D12Device> pd3dDevice)
{
	HRESULT hr = {};

	for (uint32_t i = 0; i < MAX_CMDLIST_POOL_SIZE; ++i) {
		CommandListPair cmdPair{};

		pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(cmdPair.pd3dCommandAllocator.GetAddressOf()));
		if (FAILED(hr)) {
			__debugbreak();
		}

		pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdPair.pd3dCommandAllocator.Get(), NULL, IID_PPV_ARGS(cmdPair.pd3dCommandList.GetAddressOf()));
		if (FAILED(hr)) {
			__debugbreak();
		}

		cmdPair.pd3dCommandList->Close();
		m_CmdListPool[i] = cmdPair;

		m_FreeCmdListID.push_back(i);
	}
}

CommandListPair* CommandListPool::Allocate(UINT64 ui64CompletedFenceValue)
{
	if (m_FreeCmdListID.size() == 0) {
		ReclaimEnded(ui64CompletedFenceValue);

		// 한번 Reclaim 하고도 남는게 없으면 nullptr 반환
		// 다시 Fence Value를 가져오라는 의미
		if (m_FreeCmdListID.size() == 0) {
			return nullptr;
		}
	}

	UINT nFreeID = *m_FreeCmdListID.begin();
	m_FreeCmdListID.pop_front();
	m_InUseCmdListID.push_back(nFreeID);

	auto& cmdList = m_CmdListPool[nFreeID];
	cmdList.Reset();
	cmdList.bInUse = TRUE;

	return &m_CmdListPool[nFreeID];
}

UINT64 CommandListPool::ReclaimEnded(ComPtr<ID3D12Fence> pd3dFence)
{
	UINT64 ui64CompletedValue = pd3dFence->GetCompletedValue();

	while (m_InUseCmdListID.size() != 0) {
		UINT nCurrentID = m_InUseCmdListID.front();
		if (m_CmdListPool[nCurrentID].ui64FenceValue > ui64CompletedValue) {
			break;
		}

		m_InUseCmdListID.pop_front();
		m_FreeCmdListID.push_back(nCurrentID);

		m_CmdListPool[nCurrentID].bInUse = FALSE;
	}

	return ui64CompletedValue;
}

UINT64 CommandListPool::ReclaimEnded(UINT64 ui64CompletedFenceValue)
{
	while (m_InUseCmdListID.size() != 0) {
		UINT nCurrentID = m_InUseCmdListID.front();
		if (m_CmdListPool[nCurrentID].ui64FenceValue > ui64CompletedFenceValue) {
			break;
		}

		m_InUseCmdListID.pop_front();
		m_FreeCmdListID.push_back(nCurrentID);

		m_CmdListPool[nCurrentID].bInUse = FALSE;
	}

	return ui64CompletedFenceValue;
}

BOOL CommandListPool::HasFree()
{
	return !m_FreeCmdListID.empty();
}
