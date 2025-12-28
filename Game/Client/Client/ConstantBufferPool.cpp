#include "pch.h"
#include "ConstantBufferPool.h"

ConstantBufferPool::ConstantBufferPool()
{
}

void ConstantBufferPool::Initialize(ComPtr<ID3D12Device> pd3dDevice, size_t nCBVCountIn256Bytes, UINT minCBVSize, UINT maxCBVSize)
{
	// Goal : Create CB in size of 256 -> 512 -> 768 -> 1024 -> ... -> 65536 (max size of constant buffer)
	// Number of CBV will be half of previous size's CBV Count

	HRESULT hr;
	
	UINT nCBSize = minCBVSize;				// current CB size while computing size/count
	size_t nCBVCount = nCBVCountIn256Bytes; // view count of current CB size;
	
	while (true) {
		if (nCBVCount <= 0 || nCBSize > 65536 || nCBSize > maxCBVSize) {
			nCBSize -= ConstantBufferSize<Matrix>::value;
			break;
		}

		m_nCBVCount.insert({ nCBSize, nCBVCount });
		m_CBuffers.insert({ nCBSize, {} });
		nCBVCount /= 2;
		nCBSize += ConstantBufferSize<Matrix>::value;
	}

	m_nMaxCBVSize = nCBSize;

	// Calculate required buffer size of constant buffer
	UINT ByteWidth = 0;
	for (auto& [cbSize, cbvCount] : m_nCBVCount) {
		ByteWidth += cbSize * cbvCount;
	}

	hr = pd3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(ByteWidth),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_pResource.GetAddressOf())
	);

	if (FAILED(hr)) {
		__debugbreak();
	}

	// Calculate required CBV count 
	UINT nRequiredCBVCount = 0;
	for (auto& [cbSize, cbvCount] : m_nCBVCount) {
		nRequiredCBVCount += cbvCount;
	}

	// Create descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
	{
		heapDesc.NumDescriptors = nRequiredCBVCount;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		heapDesc.NodeMask = 0;
	}

	hr = pd3dDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(m_pCBVHeap.GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
	}

	// Get Resource ptr
	CD3DX12_RANGE writeRange(0, 0);
	m_pResource->Map(0, &writeRange, reinterpret_cast<void**>(&m_pMappedPtr));

	UINT8* pMappedPtr = m_pMappedPtr;
	CD3DX12_CPU_DESCRIPTOR_HANDLE CBVHandle(m_pCBVHeap->GetCPUDescriptorHandleForHeapStart());
	UINT DescriptorSize = pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_GPU_VIRTUAL_ADDRESS GPUBufferAddr = m_pResource->GetGPUVirtualAddress();

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	
	// Create CBV per size

	int nCBVCreatedForDebug = 0;
	for (const auto& [cbSize, cbvCount] : m_nCBVCount) {
		cbvDesc.BufferLocation = GPUBufferAddr;
		cbvDesc.SizeInBytes = cbSize;

		m_CBuffers[cbSize].resize(cbvCount);
		for (int i = 0; i < cbvCount; ++i) {
			pd3dDevice->CreateConstantBufferView(&cbvDesc, CBVHandle);
			m_CBuffers[cbSize][i].CBVHandle = CBVHandle;
			m_CBuffers[cbSize][i].GPUAddress = cbvDesc.BufferLocation;
			m_CBuffers[cbSize][i].pMappedPtr = pMappedPtr;
			m_CBuffers[cbSize][i].nSize = cbSize;

			CBVHandle.Offset(1, DescriptorSize);
			cbvDesc.BufferLocation += cbSize;
			pMappedPtr += cbSize;

			nCBVCreatedForDebug++;
		}

		GPUBufferAddr += cbSize * cbvCount;
	}

	assert(nCBVCreatedForDebug == nRequiredCBVCount);

}

void ConstantBufferPool::Reset()
{
	for (auto& [CBSize, CBVAllocated] : m_nAllocated) {
		CBVAllocated = 0;
	}
}
