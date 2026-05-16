#include "pch.h"
#include "BloomPass.h"

uint32 CeilDiv(uint32 x, uint32 y)
{
	return (x + y - 1) / y;
}

void BloomPass::Initialize()
{
	CreatePipelineStates();
}

void BloomPass::OnPreRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle)
{
	RENDER->SetComputeRootSignature(pd3dCommandList.Get());
}

void BloomPass::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle)
{
	auto pHDRResult = RENDER->GetHDRBuffer(1).GetResource();
	const auto pBloomHalfBufferA = RENDER->GetPostProcessingResources().BloomHalfBuffer[0].GetResource();
	const auto pBloomHalfBufferB = RENDER->GetPostProcessingResources().BloomHalfBuffer[1].GetResource();
	auto nDescriptorInc = D3DCore::GetDescriptorIncrementSize(DESCRIPTOR_TYPE::CBV);

	constexpr auto rootParamInputTexture = std::to_underlying(COMPUTE_ROOT_PARAMETER::INPUT_SRV);
	constexpr auto rootParamOutputTexture = std::to_underlying(COMPUTE_ROOT_PARAMETER::OUTPUT_UAV);
	constexpr auto rootParamBloomData = std::to_underlying(COMPUTE_ROOT_PARAMETER::BLOOM_DATA);

	const uint32 unFullW = WinCore::g_dwClientWidth;
	const uint32 unFullH = WinCore::g_dwClientHeight;

	const uint32 unHalfW = unFullW / 2;
	const uint32 unHalfH = unFullH / 2;

	// 1. Bloom Extract
	{
		pd3dCommandList->SetPipelineState(m_pd3dBrightExtractPipelineState.Get());
		auto cpuHandle = outDescHandle.cpuHandle;
		auto gpuHandle = outDescHandle.gpuHandle;

		pHDRResult->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);			// input
		pBloomHalfBufferA->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);		// output

		DEVICE->CopyDescriptorsSimple(1, cpuHandle, pHDRResult->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		DEVICE->CopyDescriptorsSimple(1, cpuHandle.Offset(1, nDescriptorInc), pBloomHalfBufferA->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		pd3dCommandList->SetComputeRootDescriptorTable(rootParamInputTexture, gpuHandle);
		pd3dCommandList->SetComputeRootDescriptorTable(rootParamOutputTexture, gpuHandle.Offset(1, nDescriptorInc));
		outDescHandle.cpuHandle.Offset(2, nDescriptorInc);
		outDescHandle.gpuHandle.Offset(2, nDescriptorInc);

		// Set bloom data
		const auto& volume = CUR_SCENE->GetPostProcessingVolume();
		auto bloomData = volume.GetBloomCBData(XMINT2(unFullW, unFullH), XMINT2(unHalfW, unHalfH));
		auto cBuffer = RENDER->AllocCBuffer<CB_BLOOM_DATA>();
		cBuffer.WriteData(&bloomData);
		pd3dCommandList->SetComputeRootConstantBufferView(rootParamBloomData, cBuffer.GPUAddress);

		pd3dCommandList->Dispatch(
			CeilDiv(unHalfW, BLOOM_EXTRACT_THREAD_X),
			CeilDiv(unHalfH, BLOOM_EXTRACT_THREAD_Y),
			1
		);
	}

	// 2. Bloom H
	// BloomA -> BloomB
	{
		pd3dCommandList->SetPipelineState(m_pd3dBlurHPipelineState.Get());
		auto cpuHandle = outDescHandle.cpuHandle;
		auto gpuHandle = outDescHandle.gpuHandle;

		pBloomHalfBufferA->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);	// input
		pBloomHalfBufferB->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);		// output

		DEVICE->CopyDescriptorsSimple(1, cpuHandle, pBloomHalfBufferA->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		DEVICE->CopyDescriptorsSimple(1, cpuHandle.Offset(1, nDescriptorInc), pBloomHalfBufferB->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		pd3dCommandList->SetComputeRootDescriptorTable(rootParamInputTexture, gpuHandle);
		pd3dCommandList->SetComputeRootDescriptorTable(rootParamOutputTexture, gpuHandle.Offset(1, nDescriptorInc));
		outDescHandle.cpuHandle.Offset(2, nDescriptorInc);
		outDescHandle.gpuHandle.Offset(2, nDescriptorInc);

		// Set bloom data
		const auto& volume = CUR_SCENE->GetPostProcessingVolume();
		auto bloomData = volume.GetBloomCBData(XMINT2(unHalfW, unHalfW), XMINT2(unHalfW, unHalfH));
		auto cBuffer = RENDER->AllocCBuffer<CB_BLOOM_DATA>();
		cBuffer.WriteData(&bloomData);
		pd3dCommandList->SetComputeRootConstantBufferView(rootParamBloomData, cBuffer.GPUAddress);

		pd3dCommandList->Dispatch(
			CeilDiv(unHalfW, BLOOM_BLUR_THREAD_X),
			CeilDiv(unHalfH, BLOOM_BLUR_THREAD_Y),
			1
		);
	}

	// 3. Bloom V
	// BloomB -> BloomA
	{
		pd3dCommandList->SetPipelineState(m_pd3dBlurVPipelineState.Get());
		auto cpuHandle = outDescHandle.cpuHandle;
		auto gpuHandle = outDescHandle.gpuHandle;

		pBloomHalfBufferB->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);	// input
		pBloomHalfBufferA->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);		// output

		DEVICE->CopyDescriptorsSimple(1, cpuHandle, pBloomHalfBufferB->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		DEVICE->CopyDescriptorsSimple(1, cpuHandle.Offset(1, nDescriptorInc), pBloomHalfBufferA->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		pd3dCommandList->SetComputeRootDescriptorTable(rootParamInputTexture, gpuHandle);
		pd3dCommandList->SetComputeRootDescriptorTable(rootParamOutputTexture, gpuHandle.Offset(1, nDescriptorInc));
		outDescHandle.cpuHandle.Offset(2, nDescriptorInc);
		outDescHandle.gpuHandle.Offset(2, nDescriptorInc);

		// Bloom data reusable. no set.

		pd3dCommandList->Dispatch(
			CeilDiv(unHalfW, BLOOM_BLUR_THREAD_X),
			CeilDiv(unHalfH, BLOOM_BLUR_THREAD_Y),
			1
		);
	}
}

void BloomPass::OnPostRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle)
{
	// Set Bloom Result
	const auto& pBloomHalfBufferA = RENDER->GetPostProcessingResources().BloomHalfBuffer[0].GetResource();
	pBloomHalfBufferA->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);		// last output

	const uint32 unDescriptorInc = D3DCore::GetDescriptorIncrementSize(DESCRIPTOR_TYPE::CBV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle = outDescHandle.cpuHandle;
	constexpr uint32 rootParamBloom = std::to_underlying(ROOT_PARAMETER::BLOOM_RESULT);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtSRVHandle = pBloomHalfBufferA->GetSRVHandle();
	DEVICE->CopyDescriptorsSimple(1, cpuHandle, rtSRVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	cpuHandle.Offset(1, unDescriptorInc);

	pd3dCommandList->SetGraphicsRootDescriptorTable(rootParamBloom, outDescHandle.gpuHandle);
	outDescHandle.cpuHandle.Offset(1, unDescriptorInc);
	outDescHandle.gpuHandle.Offset(1, unDescriptorInc);

	// Reset root signature
	RENDER->SetGlobalRootSignature(pd3dCommandList.Get());
}

void BloomPass::CreatePipelineStates()
{
	HRESULT hr{};
	D3D12_COMPUTE_PIPELINE_STATE_DESC d3dComputePipelineDesc;

	// m_pBrightExtractPSO
	{
		d3dComputePipelineDesc.pRootSignature = RenderManager::g_pd3dComputeRootSignature.Get();
		d3dComputePipelineDesc.CS = SHADER->GetShaderByteCode("BloomBrightExtractCS");
		d3dComputePipelineDesc.CachedPSO = { nullptr, 0 };
		d3dComputePipelineDesc.NodeMask = 0;
		d3dComputePipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	}
	hr = DEVICE->CreateComputePipelineState(&d3dComputePipelineDesc, IID_PPV_ARGS(m_pd3dBrightExtractPipelineState.GetAddressOf()));

	// m_pBlurHPSO
	{
		d3dComputePipelineDesc.CS = SHADER->GetShaderByteCode("BloomBlurHorzCS");
	}
	hr = DEVICE->CreateComputePipelineState(&d3dComputePipelineDesc, IID_PPV_ARGS(m_pd3dBlurHPipelineState.GetAddressOf()));

	// m_pBlurVPSO
	{
		d3dComputePipelineDesc.CS = SHADER->GetShaderByteCode("BloomBlurVertCS");
	}
	hr = DEVICE->CreateComputePipelineState(&d3dComputePipelineDesc, IID_PPV_ARGS(m_pd3dBlurVPipelineState.GetAddressOf()));
}
