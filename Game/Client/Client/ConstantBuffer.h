#pragma once

struct ConstantBuffer
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE CBVHandle;
	D3D12_GPU_VIRTUAL_ADDRESS GPUAddress;
	UINT8* pMappedPtr;
	size_t nSize = 0;

	template<typename T>
	void WriteData(const T* pData) {
		::memcpy(pMappedPtr, pData, sizeof(T));
	}

	template<typename T>
	void WriteData(const T* pData, UINT offset) {
		T* p = reinterpret_cast<T*>(pMappedPtr);
		::memcpy(p + offset, pData, sizeof(T));
	}

	template<typename T>
	void WriteData(std::vector<T> contiguousData, UINT offset) {
		T* p = reinterpret_cast<T*>(pMappedPtr);
		::memcpy(p + offset, contiguousData.data(), sizeof(T) * contiguousData.size());
	}

	void Bind(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, UINT nRootParameterIndex) {
		pd3dCommandList->SetGraphicsRootConstantBufferView(nRootParameterIndex, GPUAddress);
	}
};

