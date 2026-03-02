#include "pch.h"
#include "StructuredBufferPool.h"

void StructuredBufferPool::Initialize(uint32 unTotalBytes, uint32 unMaxViewsPerFrame)
{
	HRESULT hr;

	m_unTotalBytes = unTotalBytes;
	m_unMaxViews = unMaxViewsPerFrame;

	m_unOffset = 0;
	m_unViewCount = 0;

	// 1. Create Upload Buffer
	hr = DEVICE->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(unTotalBytes),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_pd3dResource.GetAddressOf())
	);
	
	if (FAILED(hr)) {
		OutputDebugStringA("Failed to create structured buffer pool");
		return;
	}

	m_d3dGPUAddressBase = m_pd3dResource->GetGPUVirtualAddress();

	CD3DX12_RANGE d3dReadRange(0, 0);
	m_pd3dResource->Map(0, &d3dReadRange, reinterpret_cast<void**>(&m_pMappedPtr));

	// 2. Create Descriptor Heap
	D3D12_DESCRIPTOR_HEAP_DESC d3dHeapDesc;
	{
		d3dHeapDesc.NumDescriptors = unMaxViewsPerFrame;
		d3dHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		d3dHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		d3dHeapDesc.NodeMask = 0;
	}
	DEVICE->CreateDescriptorHeap(&d3dHeapDesc, IID_PPV_ARGS(m_pd3dDescriptorHeap.GetAddressOf()));
}

void StructuredBufferPool::CreateSRV(uint32 unOffsetBytes, uint32 unStride, uint32 unNumElements, CD3DX12_CPU_DESCRIPTOR_HANDLE destCPUHandle)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC d3dSRVDesc{};
	{
		d3dSRVDesc.Format = DXGI_FORMAT_UNKNOWN;
		d3dSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		d3dSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		d3dSRVDesc.Buffer.FirstElement = unOffsetBytes / unStride;
		d3dSRVDesc.Buffer.NumElements = unNumElements;
		d3dSRVDesc.Buffer.StructureByteStride = unStride;
		d3dSRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	}
	DEVICE->CreateShaderResourceView(m_pd3dResource.Get(), &d3dSRVDesc, destCPUHandle);
}

void StructuredBufferPool::Reset()
{
	m_unOffset = 0;
	m_unViewCount = 0;
}
