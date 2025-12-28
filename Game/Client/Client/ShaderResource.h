#pragma once

struct ShaderResource {
	ComPtr<ID3D12Resource> pResource;
	D3D12_RESOURCE_STATES currentState;

	HRESULT Create(ComPtr<ID3D12Device> pd3dDevice, const D3D12_HEAP_PROPERTIES* pHeapProperties, D3D12_HEAP_FLAGS HeapFlags, 
		const D3D12_RESOURCE_DESC* pDesc, D3D12_RESOURCE_STATES InitialResourceState, const D3D12_CLEAR_VALUE* pOptimizedClearValue) {
		
		currentState = InitialResourceState;

		return pd3dDevice->CreateCommittedResource(
			pHeapProperties,
			HeapFlags,
			pDesc,
			InitialResourceState,
			pOptimizedClearValue,
			IID_PPV_ARGS(pResource.GetAddressOf())
		);
	}

	void StateTransition(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, D3D12_RESOURCE_STATES afterstate) {
		pd3dCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pResource.Get(), currentState, afterstate, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES));
		currentState = afterstate;
	}

	D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const {
		return pResource->GetGPUVirtualAddress();
	}

};

// ========================
// Input Assembly Resources
// ========================

struct VertexBuffer {
	ShaderResource VertexBuffer;
	UINT nVertices;
	D3D12_VERTEX_BUFFER_VIEW VertexBufferView;
	UINT nType;
};

struct IndexBuffer {
	ShaderResource IndexBuffer;
	UINT nIndices;
	D3D12_INDEX_BUFFER_VIEW IndexBufferView;
};

