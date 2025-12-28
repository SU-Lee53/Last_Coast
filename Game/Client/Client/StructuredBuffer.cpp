#include "pch.h"
#include "StructuredBuffer.h"

StructuredBuffer::StructuredBuffer(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, UINT nDatas, size_t elementSize, bool bCreateView)
{
	Create(pd3dDevice, pd3dCommandList, nDatas, elementSize, bCreateView);
}

StructuredBuffer::~StructuredBuffer()
{
	m_pd3dSBuffer->Unmap(0, NULL);
}

void StructuredBuffer::Create(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, UINT nDatas, size_t elementSize, bool bCreateView)
{
	m_nDatas = nDatas;

#ifdef WITH_UPLOAD_BUFFER
	m_pd3dSBuffer = RESOURCE->CreateBufferResource(
		NULL,
		m_nDatas * elementSize,
		D3D12_HEAP_TYPE_DEFAULT,
		D3D12_RESOURCE_STATE_GENERIC_READ,
	);

	m_pd3dUploadBuffer = RESOURCE->CreateBufferResource(
		NULL,
		m_nDatas * elementSize,
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_RESOURCE_STATE_GENERIC_READ,
	);

	m_pd3dUploadBuffer->Map(0, NULL, (void**)&m_pMappedPtr);

#else
	m_pd3dSBuffer = RESOURCE->CreateBufferResource(
		NULL,
		m_nDatas * elementSize,
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_RESOURCE_STATE_GENERIC_READ
	);

	m_pd3dSBuffer->Map(0, NULL, (void**)&m_pMappedPtr);

#endif

	if (bCreateView) {
		HRESULT hr;

		D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
		{
			heapDesc.NumDescriptors = 1;
			heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			heapDesc.NodeMask = 0;
		}

		hr = pd3dDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(m_pd3dSRVHeap.GetAddressOf()));
		if (FAILED(hr)) {
			__debugbreak();
		}

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		{
			srvDesc.Format = DXGI_FORMAT_UNKNOWN;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Buffer.FirstElement = 0;
			srvDesc.Buffer.NumElements = m_nDatas;
			srvDesc.Buffer.StructureByteStride = elementSize;
			srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		}
		pd3dDevice->CreateShaderResourceView(m_pd3dSBuffer.Get(), &srvDesc, m_pd3dSRVHeap->GetCPUDescriptorHandleForHeapStart());
	}
}

void StructuredBuffer::SetBufferToPipeline(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, UINT uiRootParameterIndex) const
{
	D3D12_GPU_VIRTUAL_ADDRESS d3dGPUAddress = m_pd3dSBuffer->GetGPUVirtualAddress();
	pd3dCommandList->SetGraphicsRootShaderResourceView(uiRootParameterIndex, d3dGPUAddress);
}

D3D12_CPU_DESCRIPTOR_HANDLE StructuredBuffer::GetCPUDescriptorHandle() const
{
	return m_pd3dSRVHeap->GetCPUDescriptorHandleForHeapStart();
}

D3D12_GPU_DESCRIPTOR_HANDLE StructuredBuffer::GetGPUDescriptorHandle() const
{
	return m_pd3dSRVHeap->GetGPUDescriptorHandleForHeapStart();
}

#ifdef WITH_UPLOAD_BUFFER
void StructuredBuffer::UpdateResources(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList)
{
	D3D12_RESOURCE_BARRIER d3dResourceBarrier;
	::ZeroMemory(&d3dResourceBarrier, sizeof(D3D12_RESOURCE_BARRIER));
	d3dResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	d3dResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	d3dResourceBarrier.Transition.pResource = m_pd3dSBuffer.Get();
	d3dResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_GENERIC_READ;
	d3dResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
	d3dResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	pd3dCommandList->ResourceBarrier(1, &d3dResourceBarrier);

	pd3dCommandList->CopyResource(m_pd3dSBuffer.Get(), m_pd3dUploadBuffer.Get());

	d3dResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	d3dResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	d3dResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	pd3dCommandList->ResourceBarrier(1, &d3dResourceBarrier);

}
#endif
