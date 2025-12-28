#pragma once


//#define WITH_UPLOAD_BUFFER

class StructuredBuffer {
public:
	StructuredBuffer() = default;
	StructuredBuffer(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, UINT nDatas, size_t elementSize, bool bCreateView = false);
	~StructuredBuffer();

	void Create(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, UINT nDatas, size_t elementSize, bool bCreateView = false);

	template<typename T>
	void UpdateData(std::vector<T> data, UINT offset = 0);

	template<typename T>
	void UpdateData(const T& const data, UINT offset, UINT nDatas);

	template<typename T>
	void UpdateData(const T& const data, UINT index);

	void SetBufferToPipeline(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, UINT uiRootParameterIndex) const;

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle() const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle() const;

#ifdef WITH_UPLOAD_BUFFER
	void UpdateResources(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList);
#endif

private:
	ComPtr<ID3D12DescriptorHeap>	m_pd3dSRVHeap = nullptr;
	ComPtr<ID3D12Resource>			m_pd3dSBuffer = nullptr;
#ifdef WITH_UPLOAD_BUFFER
	ComPtr<ID3D12Resource>			m_pd3dUploadBuffer = nullptr;
#endif


	void* m_pMappedPtr = nullptr;
	UINT							m_nDatas = 0;

};

template<typename T>
inline void StructuredBuffer::UpdateData(std::vector<T> data, UINT offset)
{
	assert(data.size() < m_nDatas);
	T* pMappedPtr = reinterpret_cast<T*>(m_pMappedPtr);
	::memcpy(pMappedPtr + offset, data.data(), data.size() * sizeof(T));

}

template<typename T>
inline void StructuredBuffer::UpdateData(const T& const data, UINT offset, UINT nDatas)
{
	assert(offset + nDatas < m_nDatas);
	T* pMappedPtr = reinterpret_cast<T*>(m_pMappedPtr);
	::memcpy(pMappedPtr + offset, &data, nDatas * sizeof(T));
}

template<typename T>
inline void StructuredBuffer::UpdateData(const T& const data, UINT index)
{
	assert(index < m_nDatas);
	T* pMappedPtr = reinterpret_cast<T*>(m_pMappedPtr);
	::memcpy(pMappedPtr + index, &data, sizeof(T));
}
