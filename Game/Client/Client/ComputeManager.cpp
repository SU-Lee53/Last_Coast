#include "pch.h"
#include "ComputeManager.h"

ComputeManager::~ComputeManager()
{
	Shutdown();
}

void ComputeManager::Initialize()
{
	CreateCommandQueue();
	CreateFence();

	m_CmdAllocator.Initialize(100);
	m_DescriptorAllocator.Initialize(100);
}

void ComputeManager::Shutdown()
{
	if (m_pd3dFence && m_pd3dCommandQueue) {
		WaitForGPUComplete();
	}
	m_pd3dIndirectCommandList.clear();
	m_un64PendingFenceValues.clear();
	m_CmdAllocator.Shutdown();
	m_DescriptorAllocator.Shutdown();
	m_pd3dFence.Reset();
	m_pd3dCommandQueue.Reset();
	m_un64FenceValue = 0;
	if (m_hFenceEvent) {
		::CloseHandle(m_hFenceEvent);
		m_hFenceEvent = nullptr;
	}
}

void ComputeManager::Execute(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, uint32 unNumThreadX, uint32 unNumThreadY, uint32 unNumThreadZ) 
{
	pd3dCommandList->Close();
	ID3D12CommandList* ppd3dCommandLists[] = { pd3dCommandList.Get() };
	m_pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists);
	Fence();
}

void ComputeManager::IndirectExecute(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, uint32 unNumThreadX, uint32 unNumThreadY, uint32 unNumThreadZ)
{
	pd3dCommandList->Close();

	m_pd3dIndirectCommandList.push_back(pd3dCommandList);
}

void ComputeManager::ExecuteIndirect()
{
	std::vector<ID3D12CommandList*> pd3dCommandLists;
	pd3dCommandLists.reserve(m_pd3dIndirectCommandList.size());
	std::transform(m_pd3dIndirectCommandList.begin(), m_pd3dIndirectCommandList.end(), std::back_inserter(pd3dCommandLists), [](ComPtr<ID3D12GraphicsCommandList> p) {
		return p.Get();
	});

	m_pd3dCommandQueue->ExecuteCommandLists(pd3dCommandLists.size(), pd3dCommandLists.data());
	Fence();
}

void ComputeManager::WaitForGPUComplete() 
{
	if (m_un64PendingFenceValues.size() == 0) {
		return;
	}

	uint64 un64Expected = m_un64PendingFenceValues.back();
	WaitForFenceValue(un64Expected);

}

void ComputeManager::Reset()
{
	m_un64PendingFenceValues.clear();
	m_CmdAllocator.Reset();
	m_pd3dIndirectCommandList.clear();
}

void ComputeManager::SetDescriptorHeap(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList) const
{
	pd3dCommandList->SetDescriptorHeaps(1, m_DescriptorAllocator.GetDescriptorHeap().GetAddressOf());
}

void ComputeManager::CreateCommandQueue() {
	D3D12_COMMAND_QUEUE_DESC d3dCommandQueueDesc{};
	{
		d3dCommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		d3dCommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	}

	HRESULT hr = DEVICE->CreateCommandQueue(&d3dCommandQueueDesc, IID_PPV_ARGS(m_pd3dCommandQueue.GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
	}
}

void ComputeManager::CreateFence()
{
	DEVICE->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_pd3dFence.GetAddressOf()));
	m_hFenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
}

uint64 ComputeManager::Fence() 
{
	m_un64FenceValue++;
	m_pd3dCommandQueue->Signal(m_pd3dFence.Get(), m_un64FenceValue);
	m_un64PendingFenceValues.push_back(m_un64FenceValue);
	return m_un64FenceValue;
}

void ComputeManager::WaitForFenceValue(uint64 un64ExpectedFenceValue)
{
	if (m_pd3dFence->GetCompletedValue() < un64ExpectedFenceValue)
	{
		m_pd3dFence->SetEventOnCompletion(un64ExpectedFenceValue, m_hFenceEvent);
		WaitForSingleObject(m_hFenceEvent, INFINITE);
	}
}
