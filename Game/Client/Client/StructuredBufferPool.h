#pragma once

struct StructuredBuffer {
	D3D12_CPU_DESCRIPTOR_HANDLE SRVHandle;
	D3D12_GPU_VIRTUAL_ADDRESS GPUAddress;
	void* pMappedPtr;

	uint32 unAllocatedElements;
	uint32 unStride;

	template<typename T>
	void WriteData(const std::vector<T>& data, UINT offset = 0) {
		assert((offset + data.size()) <= unAllocatedElements);
		T* p = reinterpret_cast<T*>(pMappedPtr);
		::memcpy(p + offset, data.data(), data.size() * sizeof(T));
	}

	template<typename T>
	void WriteData(const T& data, UINT offset, UINT nDatas) {
		T* p = reinterpret_cast<T*>(pMappedPtr);
		::memcpy(p + offset, &data, nDatas * sizeof(T));
	}

	template<typename T>
	void WriteData(const T& data, UINT index) {
		T* p = reinterpret_cast<T*>(pMappedPtr);
		::memcpy(p + index, &data, sizeof(T));
	}

};

class StructuredBufferPool {
public:
	void Initialize(uint32 unTotalBytes, uint32 unMaxViewsPerFrame);

	template<typename T>
	StructuredBuffer Allocate(uint32 unNumElements);
	
	void Reset();

private:
	constexpr uint32 AlignUp(uint32 x, uint32 a) {
		return ((x + a - 1) / a) * a;
	}

	void CreateSRV(
		uint32 unOffsetBytes, 
		uint32 unStride,
		uint32 unNumElements,
		CD3DX12_CPU_DESCRIPTOR_HANDLE destCPUHandle);

private:
	ComPtr<ID3D12Resource>	m_pd3dResource;	 // Upload
	void* m_pMappedPtr;
	D3D12_GPU_VIRTUAL_ADDRESS m_d3dGPUAddressBase;

	uint32 m_unTotalBytes = 0;
	uint32 m_unOffset = 0;

	ComPtr<ID3D12DescriptorHeap> m_pd3dDescriptorHeap;
	uint32 m_unMaxViews = 0;
	uint32 m_unViewCount = 0;

};

template<typename T>
inline StructuredBuffer StructuredBufferPool::Allocate(uint32 unNumElements)
{
	uint32 unStride = sizeof(T);
	uint32 unBytes = unStride * unNumElements;

	m_unOffset = AlignUp(m_unOffset, unStride);
	assert(m_unOffset % unStride == 0);

	CD3DX12_CPU_DESCRIPTOR_HANDLE CPUHandle(m_pd3dDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	CPUHandle.Offset(m_unViewCount, D3DCore::g_nCBVSRVDescriptorIncrementSize);

	CreateSRV(m_unOffset, unStride, unNumElements, CPUHandle);

	StructuredBuffer ret;
	{
		ret.SRVHandle = CPUHandle;
		ret.GPUAddress = m_d3dGPUAddressBase + m_unOffset;
		ret.pMappedPtr = (char*)(m_pMappedPtr) + m_unOffset;

		ret.unAllocatedElements = unNumElements;
		ret.unStride = unStride;
	}

	m_unOffset += unBytes;
	m_unViewCount++;

	return ret;
}
