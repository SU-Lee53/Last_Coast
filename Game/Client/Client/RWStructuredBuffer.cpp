#include "pch.h"
#include "RWStructuredBuffer.h"

HRESULT RWStructuredBuffer::Initialize(size_t nElementSize, size_t nElementCount)
{
	HRESULT hr;
	size_t nTotalBytes = nElementSize * nElementCount;

	// 1. Descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC d3dDescriptorHeapDesc{};
	{
		d3dDescriptorHeapDesc.NumDescriptors = 2;
		d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		d3dDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		d3dDescriptorHeapDesc.NodeMask = 0;
	}
	DEVICE->CreateDescriptorHeap(&d3dDescriptorHeapDesc, IID_PPV_ARGS(&m_pd3dDescriptorHeap));

	// 2. Resource
	D3D12_RESOURCE_DESC d3dResourceDesc{};
	{
		d3dResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		d3dResourceDesc.Width = nTotalBytes;
		d3dResourceDesc.Height = 1;
		d3dResourceDesc.DepthOrArraySize = 1;
		d3dResourceDesc.MipLevels = 1;
		d3dResourceDesc.SampleDesc.Count = 1;
		d3dResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		d3dResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}

	hr = DEVICE->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&d3dResourceDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(m_pd3dResource.GetAddressOf())
	);

	if (FAILED(hr)) {
		OutputDebugStringA("Failed to create structured buffer pool");
		return hr;
	}

	m_d3dCurrentState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_pd3dDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	// 3. SRV
	D3D12_SHADER_RESOURCE_VIEW_DESC d3dSRVDesc{};
	{
		d3dSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		d3dSRVDesc.Format = DXGI_FORMAT_UNKNOWN;
		d3dSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		d3dSRVDesc.Buffer.FirstElement = 0;
		d3dSRVDesc.Buffer.NumElements = nElementCount;
		d3dSRVDesc.Buffer.StructureByteStride = nElementSize;
		d3dSRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	}
	DEVICE->CreateShaderResourceView(m_pd3dResource.Get(), &d3dSRVDesc, cpuHandle);
	m_d3dSRVHandle = cpuHandle;
	cpuHandle.ptr += D3DCore::GetDescriptorIncrementSize(DESCRIPTOR_TYPE::SRV);

	// 4. UAV
	D3D12_UNORDERED_ACCESS_VIEW_DESC d3dUAVDesc{};
	{
		d3dUAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		d3dUAVDesc.Format = DXGI_FORMAT_UNKNOWN;
		d3dUAVDesc.Buffer.FirstElement = 0;
		d3dUAVDesc.Buffer.NumElements = nElementCount;
		d3dUAVDesc.Buffer.StructureByteStride = nElementSize;
		d3dUAVDesc.Buffer.CounterOffsetInBytes = 0;
		d3dUAVDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
	}
	DEVICE->CreateUnorderedAccessView(m_pd3dResource.Get(), nullptr, &d3dUAVDesc, cpuHandle);
	m_d3dUAVHandle = cpuHandle;

	return S_OK;
}

void RWStructuredBuffer::StateTransition(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, D3D12_RESOURCE_STATES d3dAfterState)
{
	if (m_d3dCurrentState == d3dAfterState) {
		//__debugbreak();
		return;
	}

	pd3dCommandList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
			m_pd3dResource.Get(),
			m_d3dCurrentState,
			d3dAfterState,  
			D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
			D3D12_RESOURCE_BARRIER_FLAG_NONE)
	);

	m_d3dCurrentState = d3dAfterState;
}
