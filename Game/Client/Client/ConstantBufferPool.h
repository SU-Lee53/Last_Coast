#pragma once
#include "ConstantBuffer.h"

// ================================================================================
// ConstantBufferPool
// - 하나의 ID3D12Resource 를 이용하여 여러개의 ConstantBuffer 를 사용하기 위함
// - ID3D12Resource 와 ID3D12DescriptorHeap 의 쌍으로 구성 (struct ConstantBuffer)
//		- Root DescriptorHandle 로 전달하려면 바로 GPU 주소를 보낼 수 있음
//		- DescriptorHandle Table 로 전달하려면 아래의 절차를 따라야 함
//			- D3D12_DESCRIPTOR_HEAP_FLAG_NONE 이 아니므로 사용을 위해 
//			  D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE 인 DescriptorHandle Heap 에 
//			  CopyDescriptorsSimple를 수행해야 함
// - 나중에 딱 필요한 최대 크기만큼함 Pool 크기를 잡아서 사용
// ================================================================================

class ConstantBufferPool {
public:
	ConstantBufferPool();

	void Initialize(ComPtr<ID3D12Device> pd3dDevice, size_t nCBVCountIn256Bytes, UINT minCBVSize = 256, UINT maxCBVSize = 65536);

	template<typename T>
	ConstantBuffer& Allocate();
	void Reset();

private:
	//std::vector<ConstantBuffer>		m_CBuffers = {};
	
	ComPtr<ID3D12DescriptorHeap>	m_pCBVHeap = nullptr;
	ComPtr<ID3D12Resource>			m_pResource = nullptr;
	UINT8*							m_pMappedPtr = nullptr;

	std::unordered_map<size_t, std::vector<ConstantBuffer>> m_CBuffers;

	std::unordered_map<size_t, UINT>						m_nCBVCount;
	std::unordered_map<size_t, UINT>						m_nAllocated;
	size_t m_nMaxCBVSize = 0;

};

template<typename T>
inline ConstantBuffer& ConstantBufferPool::Allocate()
{
	size_t requiredCBSize = ConstantBufferSize<T>::value;
	assert(requiredCBSize <= m_nMaxCBVSize);
	assert(m_nAllocated[requiredCBSize] <= m_nCBVCount[requiredCBSize]);

	return m_CBuffers[requiredCBSize][m_nAllocated[requiredCBSize]++];


	//std::div_t sizeDevideByCBufferSize = std::div(ConstantBufferSize<T>::value, 256);
	//int nRequired = sizeDevideByCBufferSize.rem > 0 ? sizeDevideByCBufferSize.quot + 1 : sizeDevideByCBufferSize.quot;
	//UINT allocIndex = m_nAllocated;
	//m_nAllocated += nRequired;
	//return m_CBuffers[allocIndex];
}
