#pragma once
#include "D3DCore.h"

struct DescriptorHandle {
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle;
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle;

	void Bind(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, UINT nRootParameterIndex, UINT offset = 0) {
		CD3DX12_GPU_DESCRIPTOR_HANDLE bindGPUHandle;
		pd3dCommandList->SetGraphicsRootDescriptorTable(nRootParameterIndex, CD3DX12_GPU_DESCRIPTOR_HANDLE(gpuHandle, offset, D3DCore::g_nCBVSRVDescriptorIncrementSize));
	}

	std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE> Get() {
		return { cpuHandle, gpuHandle };
	}

	void CopySimpleTo(ComPtr<ID3D12Device> pd3dDevice, UINT nDescriptors, const DescriptorHandle& dest, UINT uiDestIndex, D3D12_DESCRIPTOR_HEAP_TYPE d3dHeapType = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) {
		CD3DX12_CPU_DESCRIPTOR_HANDLE d3dCPUHandleFromStart = dest.cpuHandle;
		d3dCPUHandleFromStart.Offset(uiDestIndex, D3DCore::g_nCBVSRVDescriptorIncrementSize);
		pd3dDevice->CopyDescriptorsSimple(nDescriptors, d3dCPUHandleFromStart, cpuHandle, d3dHeapType);
	}


};

class DescriptorHeap {
public:
	DescriptorHeap() = default;
	DescriptorHeap(ComPtr<ID3D12Device> pd3dDevice, D3D12_DESCRIPTOR_HEAP_DESC d3dHeapDesc);
	~DescriptorHeap();

public:
	void Initialize(ComPtr<ID3D12Device> pd3dDevice, D3D12_DESCRIPTOR_HEAP_DESC d3dHeapDesc);
	ComPtr<ID3D12DescriptorHeap>& GetD3DDescriptorHeap() { return m_pd3dDescriptorHeap; }

public:
	DescriptorHandle operator[](UINT index) {
		if (index >= m_uiCurrentDescriptorCount) {
			__debugbreak();
		}

		return DescriptorHandle{
			CD3DX12_CPU_DESCRIPTOR_HANDLE(m_DescriptorHandleFromStart.cpuHandle, index, m_uiDescriptorSize),
			CD3DX12_GPU_DESCRIPTOR_HANDLE(m_DescriptorHandleFromStart.gpuHandle, index, m_uiDescriptorSize),
		};
	}

	void Bind(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList) {
		pd3dCommandList->SetDescriptorHeaps(1, m_pd3dDescriptorHeap.GetAddressOf());
	}

	DescriptorHandle GetDescriptorHandleFromHeapStart();

private:
	ComPtr<ID3D12DescriptorHeap> m_pd3dDescriptorHeap = nullptr;

	DescriptorHandle	m_DescriptorHandleFromStart = {};
	UINT		m_uiDescriptorSize = 0;
	UINT		m_uiCurrentDescriptorCount = 0;
	UINT		m_uiAllocated = 0;

	D3D12_DESCRIPTOR_HEAP_FLAGS m_d3dHeapFlags;
	D3D12_DESCRIPTOR_HEAP_TYPE m_d3dHeapType;

};

