#include "pch.h"
#include "ResourceManager.h"
#include "Texture.h"
#include "TextureManager.h"

ResourceManager::~ResourceManager()
{
	Shutdown();
}

void ResourceManager::Initialize(ComPtr<ID3D12Device> pd3dDevice)
{
	m_pd3dDevice = pd3dDevice;
	CreateCommandList();
	CreateFence();

	m_CommandListPool.Initialize(pd3dDevice);
}

void ResourceManager::Shutdown()
{
	if (m_pd3dFence && m_pd3dCommandQueue) {
		WaitForGPUComplete();
		ReleaseCompletedUploadBuffers();
	}
	m_CommandListPool.Shutdown();
	m_pd3dFence.Reset();
	m_pd3dCommandQueue.Reset();
	m_pd3dDevice.Reset();
	m_un64FenceValue = 0;
	if (m_hFenceEvent) {
		::CloseHandle(m_hFenceEvent);
		m_hFenceEvent = nullptr;
	}
}

IndexBuffer ResourceManager::CreateIndexBuffer(const std::vector<UINT>& Indices)
{
	HRESULT hr;

	ShaderResource Buffer{};
	ComPtr<ID3D12Resource> pUploadBuffer = nullptr;
	UINT nIndices = Indices.size();
	UINT IndexBufferSize = sizeof(UINT) * nIndices;

	hr = Buffer.Create(
		DEVICE.Get(),
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(IndexBufferSize),
		D3D12_RESOURCE_STATE_COMMON,
		nullptr
	);

	if (FAILED(hr)) {
		auto hr = DEVICE->GetDeviceRemovedReason();
		__debugbreak();
	}

	if (!Indices.empty()) {
		//ResetCommandList();
		auto cmdList = AllocateCommandListSafe();

		hr = DEVICE->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(IndexBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(pUploadBuffer.GetAddressOf())
		);


		if (FAILED(hr)) {
			__debugbreak();
		}


		UINT8* pIndexDataBegin = nullptr;
		CD3DX12_RANGE readRange(0, 0);
		pUploadBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin));
		{
			memcpy(pIndexDataBegin, Indices.data(), IndexBufferSize);
		}
		pUploadBuffer->Unmap(0, nullptr);


		Buffer.StateTransition(cmdList->pd3dCommandList, D3D12_RESOURCE_STATE_COPY_DEST);
		{
			cmdList->pd3dCommandList->CopyBufferRegion(Buffer.pResource.Get(), 0, pUploadBuffer.Get(), 0, IndexBufferSize);
		}
		Buffer.StateTransition(cmdList->pd3dCommandList, D3D12_RESOURCE_STATE_INDEX_BUFFER);

		cmdList->AddPendingResource(pUploadBuffer);
		cmdList->AddPendingResource(Buffer.pResource);
		ExcuteCommandList(*cmdList);
	}

	D3D12_INDEX_BUFFER_VIEW IndexBufferView;
	IndexBufferView.BufferLocation = Buffer.GetGPUAddress();
	IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	IndexBufferView.SizeInBytes = IndexBufferSize;

	return { Buffer, nIndices, IndexBufferView };
}

ComPtr<ID3D12Resource> ResourceManager::CreateBufferResource(void* pData, UINT nBytes, D3D12_HEAP_TYPE d3dHeapType, D3D12_RESOURCE_STATES d3dResourceStates)
{
	ComPtr<ID3D12Resource> pd3dBuffer = NULL;

	CD3DX12_HEAP_PROPERTIES d3dHeapPropertiesDesc = CD3DX12_HEAP_PROPERTIES(d3dHeapType);
	CD3DX12_RESOURCE_DESC d3dResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(nBytes);

	D3D12_RESOURCE_STATES d3dResourceInitialStates = D3D12_RESOURCE_STATE_COMMON;
	if (d3dHeapType == D3D12_HEAP_TYPE_UPLOAD) d3dResourceInitialStates = D3D12_RESOURCE_STATE_GENERIC_READ;
	else if (d3dHeapType == D3D12_HEAP_TYPE_READBACK) d3dResourceInitialStates = D3D12_RESOURCE_STATE_COPY_DEST;

	HRESULT hResult = DEVICE->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(d3dHeapType),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(nBytes),
		d3dResourceInitialStates,
		NULL,
		IID_PPV_ARGS(pd3dBuffer.GetAddressOf())
	);

	if (pData)
	{
		switch (d3dHeapType)
		{
		case D3D12_HEAP_TYPE_DEFAULT:
		{
			//ResetCommandList();
			auto cmdList = AllocateCommandListSafe();

			ComPtr<ID3D12Resource> pUploadBuffer;

			d3dHeapPropertiesDesc.Type = D3D12_HEAP_TYPE_UPLOAD;
			DEVICE->CreateCommittedResource(
				&d3dHeapPropertiesDesc,
				D3D12_HEAP_FLAG_NONE,
				&d3dResourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				NULL,
				IID_PPV_ARGS(pUploadBuffer.GetAddressOf())
			);

			D3D12_RANGE d3dReadRange = { 0, 0 };
			UINT8* pBufferDataBegin = NULL;
			pUploadBuffer->Map(0, &d3dReadRange, (void**)&pBufferDataBegin);
			memcpy(pBufferDataBegin, pData, nBytes);
			pUploadBuffer->Unmap(0, NULL);

			cmdList->pd3dCommandList->CopyResource(pd3dBuffer.Get(), pUploadBuffer.Get());

			cmdList->pd3dCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pd3dBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, d3dResourceStates));
			cmdList->AddPendingResource(pUploadBuffer);
			cmdList->AddPendingResource(pd3dBuffer);
			ExcuteCommandList(*cmdList);

			break;
		}
		case D3D12_HEAP_TYPE_UPLOAD:
		{
			D3D12_RANGE d3dReadRange = { 0, 0 };
			UINT8* pBufferDataBegin = NULL;
			pd3dBuffer->Map(0, &d3dReadRange, (void**)&pBufferDataBegin);
			memcpy(pBufferDataBegin, pData, nBytes);
			pd3dBuffer->Unmap(0, NULL);
			break;
		}
		case D3D12_HEAP_TYPE_READBACK:
			break;
		}
	}

	return pd3dBuffer;
}

void ResourceManager::WaitForCopyComplete()
{
	WaitForGPUComplete();
	ReleaseCompletedUploadBuffers();
}

void ResourceManager::PollCopyComplete()
{
	ReleaseCompletedUploadBuffers();
}

bool ResourceManager::IsCopyComplete()
{
	ReleaseCompletedUploadBuffers();
	const uint64 un64LastSubmittedFenceValue = GetLastSubmittedFenceValue();
	return m_pd3dFence->GetCompletedValue() >= un64LastSubmittedFenceValue;
}

bool ResourceManager::IsFenceComplete(uint64 ui64FenceValue) const
{
	if (ui64FenceValue == 0) {
		return true;
	}

	return m_pd3dFence->GetCompletedValue() >= ui64FenceValue;
}

uint64 ResourceManager::GetLastSubmittedFenceValue() const
{
	std::lock_guard submitLock{ m_mtxSubmit };
	return m_un64FenceValue;
}

uint64 ResourceManager::GetPendingCopyFenceValue() const
{
	const uint64 un64LastSubmittedFenceValue = GetLastSubmittedFenceValue();
	const uint64 un64CompletedFenceValue = m_pd3dFence->GetCompletedValue();
	if (un64CompletedFenceValue >= un64LastSubmittedFenceValue) {
		return 0;
	}

	return un64LastSubmittedFenceValue;
}

void ResourceManager::ReleaseCompletedUploadBuffers()
{
	const uint64 un64CompletedValue = m_pd3dFence->GetCompletedValue();
	m_CommandListPool.ReclaimEnded(un64CompletedValue);
}

#pragma region D3D
void ResourceManager::CreateCommandList()
{
	HRESULT hr{};

	// Create Command Queue
	D3D12_COMMAND_QUEUE_DESC d3dCommandQueueDesc{};
	::ZeroMemory(&d3dCommandQueueDesc, sizeof(D3D12_COMMAND_QUEUE_DESC));
	{
		d3dCommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		d3dCommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	}
	hr = m_pd3dDevice->CreateCommandQueue(&d3dCommandQueueDesc, IID_PPV_ARGS(m_pd3dCommandQueue.GetAddressOf()));
	if (FAILED(hr)) {
		SHOW_ERROR("Failed to create CommandQueue");
	}
}

void ResourceManager::CreateFence()
{
	HRESULT hr{};

	hr = m_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_pd3dFence.GetAddressOf()));
	if (FAILED(hr)) {
		SHOW_ERROR("Failed to create fence");
	}

	m_hFenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
}

void ResourceManager::ExcuteCommandList(CommandListPair& cmdPair)
{
	HRESULT hr = cmdPair.pd3dCommandList->Close();
	if (FAILED(hr)) {
		SHOW_ERROR("Failed to close CommandList");
		__debugbreak();
	}

	{
		std::lock_guard submitLock{ m_mtxSubmit };

		ID3D12CommandList* ppCommandLists[] = { cmdPair.pd3dCommandList.Get() };
		m_pd3dCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
		const uint64 un64FenceValue = Fence();
		if (!m_CommandListPool.MarkSubmitted(cmdPair, un64FenceValue)) {
			assert(false && "Failed to mark cmdlist");
		}
	}
}

CommandListPair* ResourceManager::AllocateCommandListSafe()
{
	while (true) {
		const uint64 un64CompletedFenceValue = m_pd3dFence->GetCompletedValue();
		CommandListPair* pCmdList = m_CommandListPool.Allocate(un64CompletedFenceValue);

		if (pCmdList) {
			return pCmdList;
		}

		// Wait for GPU complete when cmdlist pool is full
		WaitForGPUComplete();
		std::this_thread::yield();
	}
}

uint64 ResourceManager::Fence()
{
	m_un64FenceValue++;
	m_pd3dCommandQueue->Signal(m_pd3dFence.Get(), m_un64FenceValue);

	return m_un64FenceValue;
}

void ResourceManager::WaitForGPUComplete()
{
	const uint64 un64ExpectedFenceValue = GetLastSubmittedFenceValue();
	if (m_pd3dFence->GetCompletedValue() >= un64ExpectedFenceValue) {
		return;
	}

	{
		std::lock_guard eventLock{ m_mtxFence };

		if (m_pd3dFence->GetCompletedValue() < un64ExpectedFenceValue)
		{
			m_pd3dFence->SetEventOnCompletion(un64ExpectedFenceValue, m_hFenceEvent);
			::WaitForSingleObject(m_hFenceEvent, INFINITE);
		}
	}
}

#pragma endregion D3D
