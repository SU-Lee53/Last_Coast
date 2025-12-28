#include "pch.h"
#include "ResourceManager.h"
#include "Texture.h"
#include "TextureManager.h"

ResourceManager::ResourceManager(ComPtr<ID3D12Device> pDevice)
	: m_pd3dDevice { pDevice }
{
	CreateCommandList();
	CreateFence();

	m_ConstantBufferPool.Initialize(pDevice, 1'0000);
}

ResourceManager::~ResourceManager()
{
}

IndexBuffer ResourceManager::CreateIndexBuffer(std::vector<UINT> Indices)
{
	HRESULT hr;

	ShaderResource Buffer{};
	ComPtr<ID3D12Resource> pUploadBuffer = nullptr;
	UINT nIndices = Indices.size();
	UINT IndexBufferSize = sizeof(UINT) * nIndices;

	hr = Buffer.Create(
		m_pd3dDevice,
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(IndexBufferSize),
		D3D12_RESOURCE_STATE_COMMON,
		nullptr
	);

	if (FAILED(hr)) {
		__debugbreak();
	}

	if (!Indices.empty()) {
		ResetCommandList();

		hr = m_pd3dDevice->CreateCommittedResource(
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


		Buffer.StateTransition(m_pd3dCommandList, D3D12_RESOURCE_STATE_COPY_DEST);
		{
			m_pd3dCommandList->CopyBufferRegion(Buffer.pResource.Get(), 0, pUploadBuffer.Get(), 0, IndexBufferSize);
		}
		Buffer.StateTransition(m_pd3dCommandList, D3D12_RESOURCE_STATE_INDEX_BUFFER);

		ExcuteCommandList();
		Fence();
		WaitForGPUComplete();
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

	HRESULT hResult = m_pd3dDevice->CreateCommittedResource(
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
			ResetCommandList();

			ComPtr<ID3D12Resource> pUploadBuffer;

			d3dHeapPropertiesDesc.Type = D3D12_HEAP_TYPE_UPLOAD;
			m_pd3dDevice->CreateCommittedResource(
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

			m_pd3dCommandList->CopyResource(pd3dBuffer.Get(), pUploadBuffer.Get());

			m_pd3dCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pd3dBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, d3dResourceStates));

			ExcuteCommandList();
			Fence();
			WaitForGPUComplete();
			
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

#pragma region D3D
void ResourceManager::ResetCommandList()
{
	HRESULT hr;
	hr = m_pd3dCommandAllocator->Reset();
	hr = m_pd3dCommandList->Reset(m_pd3dCommandAllocator.Get(), NULL);
	if (FAILED(hr)) {
		SHOW_ERROR("Faied to reset CommandList");
		__debugbreak();
	}
}


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

	// Create Command Allocator
	hr = m_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_pd3dCommandAllocator.GetAddressOf()));
	if (FAILED(hr)) {
		SHOW_ERROR("Failed to create CommandAllocator");
	}

	// Create Command List
	hr = m_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pd3dCommandAllocator.Get(), NULL, IID_PPV_ARGS(m_pd3dCommandList.GetAddressOf()));
	if (FAILED(hr)) {
		SHOW_ERROR("Failed to create CommandList");
	}

	// Close Command List(default is opened)
	hr = m_pd3dCommandList->Close();
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

void ResourceManager::ExcuteCommandList()
{
	HRESULT hr = m_pd3dCommandList->Close();
	if (FAILED(hr)) {
		SHOW_ERROR("Failed to close CommandList");
		__debugbreak();
	}


	ID3D12CommandList* ppCommandLists[] = { m_pd3dCommandList.Get() };
	m_pd3dCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	WaitForGPUComplete();
}

void ResourceManager::Fence()
{
	m_nFenceValue++;
	m_pd3dCommandQueue->Signal(m_pd3dFence.Get(), m_nFenceValue);
}

void ResourceManager::WaitForGPUComplete()
{

	const UINT64 expectedFenceValue = m_nFenceValue;

	if (m_pd3dFence->GetCompletedValue() < expectedFenceValue)
	{
		m_pd3dFence->SetEventOnCompletion(expectedFenceValue, m_hFenceEvent);
		::WaitForSingleObject(m_hFenceEvent, INFINITE);
	}
}

#pragma endregion D3D
