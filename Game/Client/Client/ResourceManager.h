#pragma once
#include "ConstantBufferPool.h"

// ===========================================================================================
// ResourceManager
// - Shader 변수들을 생성 및 관리
//		- VertexBuffer, IndexBuffer
//		- ConstantBuffer (ConstantBufferPool 소유)
//		- StructuredBuffer	
//		- Texture
// - 직접 생성보다 Manager 를 거쳐 생성할 것 (추후 ResourcePool 을 이용하여 관리할 예정이므로)
// 
//  10.18
//  - ShaderManager, TextureManager 별도 구현
//  - Device와 CommandList 는 ResourceManager 의 것을 같이쓴다
// ===========================================================================================

constexpr static size_t MAX_CB_POOL_SIZE = 1024;

class Texture;

class ResourceManager {
public:
	ResourceManager(ComPtr<ID3D12Device> pDevice);
	~ResourceManager();

public:
	void ResetCommandList();

public:
	template<typename T>
	VertexBuffer CreateVertexBuffer(std::vector<T> vertices, UINT nType);
	IndexBuffer CreateIndexBuffer(std::vector<UINT> Indices);

	ComPtr<ID3D12Resource> CreateBufferResource(void* pData, UINT nBytes, D3D12_HEAP_TYPE d3dHeapType, D3D12_RESOURCE_STATES d3dResourceStates);

public:
	template<typename T>
	ConstantBuffer& AllocCBuffer() {
		return m_ConstantBufferPool.Allocate<T>();
	}

	void ResetCBufferBool() {
		m_ConstantBufferPool.Reset();
	}

private:
	void CreateCommandList();
	void CreateFence();
	void Fence();
	void WaitForGPUComplete();

	void ExcuteCommandList();

private:
	ComPtr<ID3D12Device>				m_pd3dDevice = nullptr;		// Reference to D3DCore::m_pd3dDevice
	ComPtr<ID3D12GraphicsCommandList>	m_pd3dCommandList = nullptr;
	ComPtr<ID3D12CommandAllocator>		m_pd3dCommandAllocator = nullptr;
	ComPtr<ID3D12CommandQueue>			m_pd3dCommandQueue = nullptr;

	ComPtr<ID3D12Fence>		m_pd3dFence = nullptr;
	HANDLE					m_hFenceEvent = nullptr;
	UINT64					m_nFenceValue = 0;

private:
	ConstantBufferPool		m_ConstantBufferPool;

};

template<typename T>
inline VertexBuffer ResourceManager::CreateVertexBuffer(std::vector<T> vertices, UINT nType)
{
	HRESULT hr;

	ShaderResource Buffer{};
	ComPtr<ID3D12Resource> pUploadBuffer = nullptr;
	UINT nVertices = vertices.size();
	UINT VertexBufferSize = sizeof(T) * nVertices;

	CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(VertexBufferSize);

	hr = Buffer.Create(
		m_pd3dDevice,
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr
	);

	if (FAILED(hr)) {
		__debugbreak();
	}

	if (!vertices.empty()) {
		ResetCommandList();

		hr = m_pd3dDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(VertexBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(pUploadBuffer.GetAddressOf())
		);


		if (FAILED(hr)) {
			__debugbreak();
		}


		UINT8* pVertexDataBegin = nullptr;
		CD3DX12_RANGE readRange(0, 0);
		pUploadBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
		{
			memcpy(pVertexDataBegin, vertices.data(), VertexBufferSize);
		}
		pUploadBuffer->Unmap(0, nullptr);


		Buffer.StateTransition(m_pd3dCommandList, D3D12_RESOURCE_STATE_COPY_DEST);
		{
			m_pd3dCommandList->CopyBufferRegion(Buffer.pResource.Get(), 0, pUploadBuffer.Get(), 0, VertexBufferSize);
		}
		Buffer.StateTransition(m_pd3dCommandList, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

		ExcuteCommandList();
		Fence();
		WaitForGPUComplete();
	}

	D3D12_VERTEX_BUFFER_VIEW VertexBufferView;
	VertexBufferView.BufferLocation = Buffer.GetGPUAddress();
	VertexBufferView.StrideInBytes = sizeof(T);
	VertexBufferView.SizeInBytes = VertexBufferSize;

	VertexBuffer ret;
	ret.VertexBuffer = Buffer;
	ret.nVertices = nVertices;
	ret.VertexBufferView = VertexBufferView;
	ret.nType = nType;


	return ret;
}
