#pragma once
#include "ConstantBuffer.h"
#include <bit>
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

template<typename T>
constexpr size_t ComputeCBSize()
{
	return sizeof(T) <= 256 ? 256 : std::bit_ceil(sizeof(T));
}

template<typename T>
struct ConstantBufferPoolSize {
	constexpr static size_t value = ComputeCBSize<T>();
};

class ConstantBufferPool {
public:
	ConstantBufferPool();

	void Initialize(size_t nCBVCountIn256Bytes, UINT minCBVSize = 256, UINT maxCBVSize = 65536);

	template<typename T>
	ConstantBuffer& Allocate();
	void Reset();
	void Shutdown();

	void ShowDebugInfo();

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
	size_t requiredCBSize = ConstantBufferPoolSize<T>::value;
	assert(requiredCBSize <= m_nMaxCBVSize);

	// 기존 assert는 <= 라서 개수와 같아지는 순간(=마지막+1) OOB 인덱싱을 통과시켰고,
	// Release에선 assert 자체가 사라져 조용히 벡터 밖을 읽었다 — 간헐적 크래시/GPU 오염 원인 후보.
	if (m_nAllocated[requiredCBSize] >= m_nCBVCount[requiredCBSize]) {
		char szBuf[160];
		sprintf_s(szBuf, "[ConstantBufferPool] OVERFLOW! size=%zu allocated=%u/%u\n",
			requiredCBSize, m_nAllocated[requiredCBSize], m_nCBVCount[requiredCBSize]);
		D3DCore::AppendCrashLog(szBuf);
		__debugbreak();
		m_nAllocated[requiredCBSize] = 0;	// 최소 방어 — OOB 대신 풀 앞부분 재사용
	}

	return m_CBuffers[requiredCBSize][m_nAllocated[requiredCBSize]++];


	//std::div_t sizeDevideByCBufferSize = std::div(ConstantBufferSize<T>::value, 256);
	//int nRequired = sizeDevideByCBufferSize.rem > 0 ? sizeDevideByCBufferSize.quot + 1 : sizeDevideByCBufferSize.quot;
	//UINT allocIndex = m_nAllocated;
	//m_nAllocated += nRequired;
	//return m_CBuffers[allocIndex];
}
