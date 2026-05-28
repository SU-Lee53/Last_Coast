#include "pch.h"
#include "BloomPass.h"

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

	const auto& volume = CUR_SCENE->GetPostProcessingVolume();

	const auto& pBloomHalfBuffers =		RENDER->GetPostProcessingResources().BloomHalfBuffer;
	const auto& pBloomQuaterBuffers =	RENDER->GetPostProcessingResources().BloomQuaterBuffer;
	const auto& pBloomEighthBuffers =	RENDER->GetPostProcessingResources().BloomEighthBuffer;

	auto nDescriptorInc = D3DCore::GetDescriptorIncrementSize(DESCRIPTOR_TYPE::CBV);

	constexpr auto rootParamInputTexture = std::to_underlying(COMPUTE_ROOT_PARAMETER::INPUT_SRV_FLOAT4);
	constexpr auto rootParamOutputTexture = std::to_underlying(COMPUTE_ROOT_PARAMETER::OUTPUT_UAV_FLOAT4);
	constexpr auto rootParamBloomData = std::to_underlying(COMPUTE_ROOT_PARAMETER::BLOOM_DATA);

	const uint32 unFullW = WinCore::g_dwClientWidth;
	const uint32 unFullH = WinCore::g_dwClientHeight;

	const uint32 unTargetWidths[4] = { unFullW, unFullW / 2, unFullW / 4, unFullW / 8 };
	const uint32 unTargetHeights[4] = { unFullH, unFullH / 2, unFullH / 4, unFullH / 8 };

	// 1. Bloom Extract
	{
		pd3dCommandList->SetPipelineState(m_pd3dBrightExtractPipelineState.Get());
		auto cpuHandle = outDescHandle.cpuHandle;
		auto gpuHandle = outDescHandle.gpuHandle;

		// Half
		{
			auto& pInput = pHDRResult;
			auto& pOutput = pBloomHalfBuffers[0].GetResource();

			pInput->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);			// input
			pOutput->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);		// output

			DEVICE->CopyDescriptorsSimple(1, cpuHandle, pInput->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			DEVICE->CopyDescriptorsSimple(1, cpuHandle.Offset(1, nDescriptorInc), pOutput->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			pd3dCommandList->SetComputeRootDescriptorTable(rootParamInputTexture, gpuHandle);
			pd3dCommandList->SetComputeRootDescriptorTable(rootParamOutputTexture, gpuHandle.Offset(1, nDescriptorInc));
			outDescHandle.cpuHandle.Offset(2, nDescriptorInc);
			outDescHandle.gpuHandle.Offset(2, nDescriptorInc);

			// Set bloom data
			auto bloomData = volume.GetBloomCBData(XMINT2(unFullW, unFullH), XMINT2(unTargetWidths[1], unTargetHeights[1]));
			auto cBuffer = RENDER->AllocCBuffer<CB_BLOOM_DATA>();
			cBuffer.WriteData(&bloomData);
			pd3dCommandList->SetComputeRootConstantBufferView(rootParamBloomData, cBuffer.GPUAddress);

			pd3dCommandList->Dispatch(
				::CeilDiv(unTargetWidths[1], BLOOM_EXTRACT_THREAD_X),
				::CeilDiv(unTargetHeights[1], BLOOM_EXTRACT_THREAD_Y),
				1
			);
		}

		// Quater
		{
			auto& pInput = pBloomHalfBuffers[0].GetResource();
			auto& pOutput = pBloomQuaterBuffers[0].GetResource();

			pInput->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);			// input
			pOutput->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);		// output

			DEVICE->CopyDescriptorsSimple(1, cpuHandle, pInput->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			DEVICE->CopyDescriptorsSimple(1, cpuHandle.Offset(1, nDescriptorInc), pOutput->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			pd3dCommandList->SetComputeRootDescriptorTable(rootParamInputTexture, gpuHandle);
			pd3dCommandList->SetComputeRootDescriptorTable(rootParamOutputTexture, gpuHandle.Offset(1, nDescriptorInc));
			outDescHandle.cpuHandle.Offset(2, nDescriptorInc);
			outDescHandle.gpuHandle.Offset(2, nDescriptorInc);

			// Set bloom data
			auto bloomData = volume.GetBloomCBData(XMINT2(unTargetWidths[1], unTargetHeights[1]), XMINT2(unTargetWidths[2], unTargetHeights[2]));
			auto cBuffer = RENDER->AllocCBuffer<CB_BLOOM_DATA>();
			cBuffer.WriteData(&bloomData);
			pd3dCommandList->SetComputeRootConstantBufferView(rootParamBloomData, cBuffer.GPUAddress);

			pd3dCommandList->Dispatch(
				::CeilDiv(unTargetWidths[2], BLOOM_EXTRACT_THREAD_X),
				::CeilDiv(unTargetHeights[2], BLOOM_EXTRACT_THREAD_Y),
				1
			);
		}

		// Eighth
		{
			auto& pInput = pBloomQuaterBuffers[0].GetResource();
			auto& pOutput = pBloomEighthBuffers[0].GetResource();

			pInput->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);			// input
			pOutput->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);		// output

			DEVICE->CopyDescriptorsSimple(1, cpuHandle, pInput->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			DEVICE->CopyDescriptorsSimple(1, cpuHandle.Offset(1, nDescriptorInc), pOutput->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			pd3dCommandList->SetComputeRootDescriptorTable(rootParamInputTexture, gpuHandle);
			pd3dCommandList->SetComputeRootDescriptorTable(rootParamOutputTexture, gpuHandle.Offset(1, nDescriptorInc));
			outDescHandle.cpuHandle.Offset(2, nDescriptorInc);
			outDescHandle.gpuHandle.Offset(2, nDescriptorInc);

			// Set bloom data
			auto bloomData = volume.GetBloomCBData(XMINT2(unTargetWidths[2], unTargetHeights[2]), XMINT2(unTargetWidths[3], unTargetHeights[3]));
			auto cBuffer = RENDER->AllocCBuffer<CB_BLOOM_DATA>();
			cBuffer.WriteData(&bloomData);
			pd3dCommandList->SetComputeRootConstantBufferView(rootParamBloomData, cBuffer.GPUAddress);

			pd3dCommandList->Dispatch(
				::CeilDiv(unTargetWidths[3], BLOOM_EXTRACT_THREAD_X),
				::CeilDiv(unTargetHeights[3], BLOOM_EXTRACT_THREAD_Y),
				1
			);
		}
		
	}

	// 2. Bloom H
	// BloomA -> BloomB
	{
		pd3dCommandList->SetPipelineState(m_pd3dBlurHPipelineState.Get());
		for (int i = 0; i < 3; ++i) {
			auto cpuHandle = outDescHandle.cpuHandle;
			auto gpuHandle = outDescHandle.gpuHandle;

			auto& pInput = (i == 0) ? pBloomHalfBuffers[0].GetResource() 
									: (i == 1) ? pBloomQuaterBuffers[0].GetResource() 
											   : pBloomEighthBuffers[0].GetResource();

			auto& pOutput = (i == 0) ? pBloomHalfBuffers[1].GetResource()
									 : (i == 1) ? pBloomQuaterBuffers[1].GetResource() 
											    : pBloomEighthBuffers[1].GetResource();

			pInput->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);	// input
			pOutput->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);		// output

			DEVICE->CopyDescriptorsSimple(1, cpuHandle, pInput->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			DEVICE->CopyDescriptorsSimple(1, cpuHandle.Offset(1, nDescriptorInc), pOutput->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			pd3dCommandList->SetComputeRootDescriptorTable(rootParamInputTexture, gpuHandle);
			pd3dCommandList->SetComputeRootDescriptorTable(rootParamOutputTexture, gpuHandle.Offset(1, nDescriptorInc));
			outDescHandle.cpuHandle.Offset(2, nDescriptorInc);
			outDescHandle.gpuHandle.Offset(2, nDescriptorInc);

			// Set bloom data
			auto bloomData = volume.GetBloomCBData(XMINT2(unTargetWidths[i+1], unTargetHeights[i + 1]), XMINT2(unTargetWidths[i + 1], unTargetHeights[i + 1]));
			auto cBuffer = RENDER->AllocCBuffer<CB_BLOOM_DATA>();
			cBuffer.WriteData(&bloomData);
			pd3dCommandList->SetComputeRootConstantBufferView(rootParamBloomData, cBuffer.GPUAddress);

			pd3dCommandList->Dispatch(
				::CeilDiv(unTargetWidths[i + 1], BLOOM_BLUR_THREAD_X),
				::CeilDiv(unTargetHeights[i + 1], BLOOM_BLUR_THREAD_Y),
				1
			);
		}
	}

	// 3. Bloom V
	// BloomB -> BloomA
	{
		pd3dCommandList->SetPipelineState(m_pd3dBlurVPipelineState.Get());
		auto cpuHandle = outDescHandle.cpuHandle;
		auto gpuHandle = outDescHandle.gpuHandle;

		for (int i = 0; i < 3; ++i) {
			auto cpuHandle = outDescHandle.cpuHandle;
			auto gpuHandle = outDescHandle.gpuHandle;

			auto& pInput = (i == 0) ? pBloomHalfBuffers[1].GetResource()
									: (i == 1) ? pBloomQuaterBuffers[1].GetResource()
											   : pBloomEighthBuffers[1].GetResource();

			auto& pOutput = (i == 0) ? pBloomHalfBuffers[0].GetResource()
									 : (i == 1) ? pBloomQuaterBuffers[0].GetResource()
												: pBloomEighthBuffers[0].GetResource();

			pInput->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);	// input
			pOutput->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);		// output

			DEVICE->CopyDescriptorsSimple(1, cpuHandle, pInput->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			DEVICE->CopyDescriptorsSimple(1, cpuHandle.Offset(1, nDescriptorInc), pOutput->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			pd3dCommandList->SetComputeRootDescriptorTable(rootParamInputTexture, gpuHandle);
			pd3dCommandList->SetComputeRootDescriptorTable(rootParamOutputTexture, gpuHandle.Offset(1, nDescriptorInc));
			outDescHandle.cpuHandle.Offset(2, nDescriptorInc);
			outDescHandle.gpuHandle.Offset(2, nDescriptorInc);

			// Set bloom data
			auto bloomData = volume.GetBloomCBData(XMINT2(unTargetWidths[i + 1], unTargetHeights[i + 1]), XMINT2(unTargetWidths[i + 1], unTargetHeights[i + 1]));
			auto cBuffer = RENDER->AllocCBuffer<CB_BLOOM_DATA>();
			cBuffer.WriteData(&bloomData);
			pd3dCommandList->SetComputeRootConstantBufferView(rootParamBloomData, cBuffer.GPUAddress);

			pd3dCommandList->Dispatch(
				CeilDiv(unTargetWidths[i + 1], BLOOM_BLUR_THREAD_X),
				CeilDiv(unTargetHeights[i + 1], BLOOM_BLUR_THREAD_Y),
				1
			);
		}
	}
}

void BloomPass::OnPostRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle)
{
	// Set Bloom Result
	const auto pFinalOutputHalf = RENDER->GetPostProcessingResources().BloomHalfBuffer[0].GetResource();
	const auto pFinalOutputQuater = RENDER->GetPostProcessingResources().BloomQuaterBuffer[0].GetResource();
	const auto pFinalOutputEighth = RENDER->GetPostProcessingResources().BloomEighthBuffer[0].GetResource();
	pFinalOutputHalf->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);		// last half output
	pFinalOutputQuater->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);		// last quater output
	pFinalOutputEighth->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);		// last eighth output

	const uint32 unDescriptorInc = D3DCore::GetDescriptorIncrementSize(DESCRIPTOR_TYPE::CBV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle = outDescHandle.cpuHandle;
	constexpr uint32 rootParamBloom = std::to_underlying(ROOT_PARAMETER::BLOOM_RESULT);

	DEVICE->CopyDescriptorsSimple(1, cpuHandle, pFinalOutputHalf->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	DEVICE->CopyDescriptorsSimple(1, cpuHandle.Offset(1, unDescriptorInc), pFinalOutputQuater->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	DEVICE->CopyDescriptorsSimple(1, cpuHandle.Offset(1, unDescriptorInc), pFinalOutputEighth->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pd3dCommandList->SetGraphicsRootDescriptorTable(rootParamBloom, outDescHandle.gpuHandle);
	outDescHandle.cpuHandle.Offset(3, unDescriptorInc);
	outDescHandle.gpuHandle.Offset(3, unDescriptorInc);

	// Reset root signature
	//RENDER->SetGlobalRootSignature(pd3dCommandList.Get());
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
