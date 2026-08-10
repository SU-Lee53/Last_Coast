#pragma once

class DescriptorAllocator {
public:
	void Initialize(uint32 nDescriptorCount) {
		m_unCount = nDescriptorCount;

		D3D12_DESCRIPTOR_HEAP_DESC d3dHeapDesc{};
		{
			d3dHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			d3dHeapDesc.NumDescriptors = nDescriptorCount;
			d3dHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			d3dHeapDesc.NodeMask = 0;
		}

		HRESULT hr = DEVICE->CreateDescriptorHeap(&d3dHeapDesc, IID_PPV_ARGS(m_pd3dDescriptorHeap.GetAddressOf()));
		if (FAILED(hr)) {
			__debugbreak();
		}

		m_unDescriptorIncrementSize = DEVICE->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	DescriptorHandle Allocate() {
		uint32 offset = m_unAllocated++;

		CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(m_pd3dDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
		CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(m_pd3dDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

		return DescriptorHandle{
			cpuHandle.Offset(offset, m_unDescriptorIncrementSize),
			gpuHandle.Offset(offset, m_unDescriptorIncrementSize)
		};
	}

	void Reset() {
		m_unAllocated = 0;
	}

	void Shutdown() {
		m_pd3dDescriptorHeap.Reset();
		m_unCount = 0;
		m_unAllocated = 0;
		m_unDescriptorIncrementSize = 0;
	}

	ComPtr<ID3D12DescriptorHeap> GetDescriptorHeap() const { return m_pd3dDescriptorHeap; }

private:
	ComPtr<ID3D12DescriptorHeap> m_pd3dDescriptorHeap;
	uint32 m_unCount = 0;
	uint32 m_unAllocated = 0;
	uint32 m_unDescriptorIncrementSize = 0;
};


class ComputeManager {

	DECLARE_SINGLE(ComputeManager);
	~ComputeManager();

public:
	void Initialize();
	void Shutdown();
	void Execute(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, uint32 unNumThreadX, uint32 unNumThreadY, uint32 unNumThreadZ);
	void IndirectExecute(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, uint32 unNumThreadX, uint32 unNumThreadY, uint32 unNumThreadZ);

	void ExecuteIndirect();
	void WaitForGPUComplete();

	void Reset();

	CommandListAllocator::CommandListPair& AllocCommandList() {
		return m_CmdAllocator.Allocate();
	}

	DescriptorHandle AllocDescriptor() {
		return m_DescriptorAllocator.Allocate();
	}

	void SetDescriptorHeap(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList) const;

private:
	void CreateCommandQueue();
	void CreateFence();
	
	uint64 Fence();
	void WaitForFenceValue(uint64 un64ExpectedFenceValue);

private:
	ComPtr<ID3D12CommandQueue> m_pd3dCommandQueue;
	CommandListAllocator m_CmdAllocator;
	DescriptorAllocator m_DescriptorAllocator;

	ComPtr<ID3D12Fence> m_pd3dFence;
	HANDLE m_hFenceEvent = nullptr;
	uint64 m_un64FenceValue = 0;
	std::vector<uint64> m_un64PendingFenceValues;

	std::vector<ComPtr<ID3D12GraphicsCommandList>> m_pd3dIndirectCommandList;
};

