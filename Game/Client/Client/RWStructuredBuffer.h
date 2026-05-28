#pragma once

class RWStructuredBuffer {
public:
	HRESULT Initialize(size_t nElementSize, size_t nElementCount);

	void StateTransition(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		D3D12_RESOURCE_STATES d3dAfterState);

	D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddr() const { return m_pd3dResource->GetGPUVirtualAddress(); }
	D3D12_CPU_DESCRIPTOR_HANDLE GetSRVHandle() const { return m_d3dSRVHandle; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetUAVHandle() const { return m_d3dUAVHandle; }

private:
	ComPtr<ID3D12Resource> m_pd3dResource;
	ComPtr<ID3D12DescriptorHeap> m_pd3dDescriptorHeap;

	D3D12_CPU_DESCRIPTOR_HANDLE m_d3dSRVHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE m_d3dUAVHandle;

	D3D12_RESOURCE_STATES m_d3dCurrentState{};

};
