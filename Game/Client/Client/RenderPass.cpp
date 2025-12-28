#include "pch.h"
#include "RenderPass.h"
#include "Mesh.h"
#include "Transform.h"
#include "RenderManager.h"	// for InstancePair

ForwardPass::ForwardPass(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommmandList)
{
	m_InstanceSBuffer.Create(pd3dDevice, pd3dCommmandList, 100000, sizeof(Matrix), true);
}

void ForwardPass::Run(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, std::span<InstancePair> instances, DescriptorHandle& descHandleFromPassStart)
{
	UINT uiSBufferOffset = 0;
	for (const auto& [k, v] : instances) {
		m_InstanceSBuffer.UpdateData(v, uiSBufferOffset);
		uiSBufferOffset += v.size();
	}

#ifdef WITH_UPLOAD_BUFFER
	m_InstanceDataSBuffer.UpdateResources(m_pd3dDevice, pd3dCommandList);

#endif

	pd3dDevice->CopyDescriptorsSimple(1, descHandleFromPassStart.cpuHandle,
		m_InstanceSBuffer.GetCPUDescriptorHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	descHandleFromPassStart.cpuHandle.Offset(1, D3DCore::g_nCBVSRVDescriptorIncrementSize);

	// 3
	pd3dCommandList->SetGraphicsRootDescriptorTable(ROOT_PARAMETER_PASS_INSTANCE_DATA, descHandleFromPassStart.gpuHandle);
	descHandleFromPassStart.gpuHandle.Offset(1, D3DCore::g_nCBVSRVDescriptorIncrementSize);
	
	int nInstanceBase = 0;
	for (auto& instance : instances) {
		instance.meshRenderer->Render(pd3dDevice, pd3dCommandList, descHandleFromPassStart, instance.InstanceDatas.size(), nInstanceBase);
	}
}
