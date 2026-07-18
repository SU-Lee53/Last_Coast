#include "pch.h"
#include "CommandListPool.h"

void CommandListPool::Initialize(ComPtr<ID3D12Device> pd3dDevice)
{
	HRESULT hr = {};
	uint64 un64AvailableMask = 0;

	for (uint32 i = 0; i < MAX_CMDLIST_POOL_SIZE; ++i) {
		auto& cmdPair = m_CmdListPool[i];
		cmdPair.unPoolIndex = i;
		cmdPair.ui64FenceValue = 0;

		hr = pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(cmdPair.pd3dCommandAllocator.GetAddressOf()));
		if (FAILED(hr)) {
			__debugbreak();
		}

		hr = pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdPair.pd3dCommandAllocator.Get(), NULL, IID_PPV_ARGS(cmdPair.pd3dCommandList.GetAddressOf()));
		if (FAILED(hr)) {
			__debugbreak();
		}

		cmdPair.pd3dCommandList->Close();
		cmdPair.eState.store(COMMAND_LIST_STATE::FREE);

		un64AvailableMask |= uint64{ 1 } << i;
	}

	m_un64FreeMask.store(un64AvailableMask);
}

CommandListPair* CommandListPool::Allocate(uint64 ui64CompletedFenceValue)
{
	ReclaimEnded(ui64CompletedFenceValue);

	uint64 un64FreeMask = m_un64FreeMask.load();
	while (un64FreeMask != 0) {
		// Free mask recored free cmdlists using bits
		// Index starts from right (lsb)
		// 1 means free, 0 means in use
		// Which means : Consecutive 0 counts means free cmdlist index
		// 1101100 -> countr_zero() returns 2, m_CmdListPool[2] is free now
		// 1000001 -> countr_zero() returns 0, m_CmdListPool[0] is free now
		const uint32 unPoolIndex = static_cast<uint32>(std::countr_zero(un64FreeMask));

		// signs bit using CAS
		const uint64 un64CurBit = uint64{ 1 } << unPoolIndex;
		const uint64 un64NewMask = un64FreeMask & ~un64CurBit;
		if (!m_un64FreeMask.compare_exchange_strong(un64FreeMask, un64NewMask)) {
			continue;
		}

		auto& cmdPair = m_CmdListPool[unPoolIndex];
		assert(cmdPair.eState.load() == COMMAND_LIST_STATE::FREE && "Wront Allocation");

		cmdPair.eState.store(COMMAND_LIST_STATE::RECORDING);
		cmdPair.ui64FenceValue = 0;
		HRESULT hr = cmdPair.Reset();
		if (FAILED(hr)) {
			ReturnToFree(unPoolIndex);
			return nullptr;
		}

		return &cmdPair;
	}

	return nullptr;
}

uint64 CommandListPool::ReclaimEnded(ComPtr<ID3D12Fence> pd3dFence)
{
	const uint64 un64CompletedFenceValue =
		pd3dFence->GetCompletedValue();

	return ReclaimEnded(un64CompletedFenceValue);
}

uint64 CommandListPool::ReclaimEnded(uint64 ui64CompletedFenceValue)
{
	for (uint32 i = 0; i < MAX_CMDLIST_POOL_SIZE; ++i) {
		auto& cmdList = m_CmdListPool[i];
		if (cmdList.eState.load() != COMMAND_LIST_STATE::SUBMITTED) {
			continue;
		}

		if (cmdList.ui64FenceValue > ui64CompletedFenceValue) {
			continue;
		}

		auto eExpected = COMMAND_LIST_STATE::SUBMITTED;
		if (!cmdList.eState.compare_exchange_strong(eExpected, COMMAND_LIST_STATE::RECLAIMING)) {
			continue;
		}

		cmdList.ui64FenceValue = 0;
		ReturnToFree(i);
	}

	return ui64CompletedFenceValue;
}

bool CommandListPool::MarkSubmitted(CommandListPair& cmdPair, uint64 un64FenceValue)
{
	if (un64FenceValue == 0) {
		return false;
	}

	if (cmdPair.eState.load() != COMMAND_LIST_STATE::RECORDING) {
		return false;
	}

	cmdPair.ui64FenceValue = un64FenceValue;
	cmdPair.eState.store(COMMAND_LIST_STATE::SUBMITTED);

	return true;
}

BOOL CommandListPool::HasFree()
{
	return m_un64FreeMask.load() != 0;
}

void CommandListPool::ReturnToFree(UINT unPoolIndex)
{
	auto& cmdList = m_CmdListPool[unPoolIndex];
	cmdList.m_PendingUploadBuffers.clear();
	cmdList.ui64FenceValue = 0;


	cmdList.eState.store(COMMAND_LIST_STATE::FREE);
	m_un64FreeMask.fetch_or(uint64{ 1 } << unPoolIndex);
}
