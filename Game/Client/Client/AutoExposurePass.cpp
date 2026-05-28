#include "pch.h"
#include "AutoExposurePass.h"

void AutoExposurePass::Initialize()
{
	CreatePipelineStates();
}

void AutoExposurePass::OnPreRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle)
{
	RENDER->SetComputeRootSignature(pd3dCommandList.Get());
}

void AutoExposurePass::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle)
{
	auto pHDRResult = RENDER->GetHDRBuffer(1).GetResource();

	const auto& volume = CUR_SCENE->GetPostProcessingVolume();

	const auto& LuminanceBuffers = RENDER->GetPostProcessingResources().LuminanceBuffer;
	const auto& LuminanceFinalBuffer = RENDER->GetPostProcessingResources().LuminanceFinalBuffer;
	auto nDescriptorInc = D3DCore::GetDescriptorIncrementSize(DESCRIPTOR_TYPE::CBV);

	constexpr auto rootParamInputTexture = std::to_underlying(COMPUTE_ROOT_PARAMETER::INPUT_SRV_FLOAT);
	constexpr auto rootParamOutputTexture = std::to_underlying(COMPUTE_ROOT_PARAMETER::OUTPUT_UAV_FLOAT);
	constexpr auto rootParamCBData = std::to_underlying(COMPUTE_ROOT_PARAMETER::AUTO_EXPOSURE_DATA);

	const uint32 unFullW = WinCore::g_dwClientWidth;
	const uint32 unFullH = WinCore::g_dwClientHeight;

	const std::array<uint32, 5> unTargetWidths = { unFullW, unFullW / 2, unFullW / 4, unFullW / 8, unFullW / 16 };
	const std::array<uint32, 5> unTargetHeights = { unFullH, unFullH / 2, unFullH / 4, unFullH / 8, unFullH / 16 };

	// 1. Extract
	{
		auto cpuHandle = outDescHandle.cpuHandle;
		auto gpuHandle = outDescHandle.gpuHandle;
		pd3dCommandList->SetPipelineState(m_pd3dLuminanceExtractPipelineState.Get());
		auto& pInput = pHDRResult;
		auto& pOutput = LuminanceBuffers[0].GetResource();

		pInput->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);			// input
		pOutput->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);		// output

		DEVICE->CopyDescriptorsSimple(1, cpuHandle, pInput->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		DEVICE->CopyDescriptorsSimple(1, cpuHandle.Offset(1, nDescriptorInc), pOutput->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		pd3dCommandList->SetComputeRootDescriptorTable(rootParamInputTexture, gpuHandle);
		pd3dCommandList->SetComputeRootDescriptorTable(rootParamOutputTexture, gpuHandle.Offset(1, nDescriptorInc));
		outDescHandle.cpuHandle.Offset(2, nDescriptorInc);
		outDescHandle.gpuHandle.Offset(2, nDescriptorInc);

		// Set bloom data
		auto data = CB_AUTO_EXPOSURE_INOUT_DATA{ 
			XMINT2(unTargetWidths[0], unTargetHeights[0]), 
			XMINT2(unTargetWidths[1], unTargetHeights[1]) 
		};
		auto cBuffer = RENDER->AllocCBuffer<CB_AUTO_EXPOSURE_INOUT_DATA>();
		cBuffer.WriteData(&data);
		pd3dCommandList->SetComputeRootConstantBufferView(rootParamCBData, cBuffer.GPUAddress);

		pd3dCommandList->Dispatch(
			CeilDiv(unTargetWidths[1], THREAD_X),
			CeilDiv(unTargetHeights[1], THREAD_Y),
			1
		);
	}

	// 2. Reduce
	{
		pd3dCommandList->SetPipelineState(m_pd3dReduceLuminancePipelineState.Get());
		for (int i = 0; i < LuminanceBuffers.size() - 1; ++i) {
			auto cpuHandle = outDescHandle.cpuHandle;
			auto gpuHandle = outDescHandle.gpuHandle;

			auto& pInput = LuminanceBuffers[i].GetResource();
			auto& pOutput = LuminanceBuffers[i + 1].GetResource();

			pInput->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);			// input
			pOutput->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);		// output

			DEVICE->CopyDescriptorsSimple(1, cpuHandle, pInput->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			DEVICE->CopyDescriptorsSimple(1, cpuHandle.Offset(1, nDescriptorInc), pOutput->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			pd3dCommandList->SetComputeRootDescriptorTable(rootParamInputTexture, gpuHandle);
			pd3dCommandList->SetComputeRootDescriptorTable(rootParamOutputTexture, gpuHandle.Offset(1, nDescriptorInc));
			outDescHandle.cpuHandle.Offset(2, nDescriptorInc);
			outDescHandle.gpuHandle.Offset(2, nDescriptorInc);

			auto data = CB_AUTO_EXPOSURE_INOUT_DATA{
				XMINT2(unTargetWidths[i + 1], unTargetHeights[i + 1]),
				XMINT2(unTargetWidths[i + 2], unTargetHeights[i + 2])
			};
			auto cBuffer = RENDER->AllocCBuffer<CB_AUTO_EXPOSURE_INOUT_DATA>();
			cBuffer.WriteData(&data);
			pd3dCommandList->SetComputeRootConstantBufferView(rootParamCBData, cBuffer.GPUAddress);

			pd3dCommandList->Dispatch(
				CeilDiv(unTargetWidths[i + 2], THREAD_X),
				CeilDiv(unTargetHeights[i + 2], THREAD_Y),
				1
			);
		}
	}


	// 3. Final 1x1
	{
		auto cpuHandle = outDescHandle.cpuHandle;
		auto gpuHandle = outDescHandle.gpuHandle;

		auto& pInput = LuminanceBuffers.back().GetResource();
		auto& pOutput = LuminanceFinalBuffer.GetResource();

		pInput->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);		// input
		pOutput->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);		// output

		DEVICE->CopyDescriptorsSimple(1, cpuHandle, pInput->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		DEVICE->CopyDescriptorsSimple(1, cpuHandle.Offset(1, nDescriptorInc), pOutput->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		pd3dCommandList->SetComputeRootDescriptorTable(rootParamInputTexture, gpuHandle);
		pd3dCommandList->SetComputeRootDescriptorTable(rootParamOutputTexture, gpuHandle.Offset(1, nDescriptorInc));
		outDescHandle.cpuHandle.Offset(2, nDescriptorInc);
		outDescHandle.gpuHandle.Offset(2, nDescriptorInc);

		auto data = CB_AUTO_EXPOSURE_INOUT_DATA{
			XMINT2(unTargetWidths.back(), unTargetHeights.back()),
			XMINT2(1, 1)
		};
		auto cBuffer = RENDER->AllocCBuffer<CB_AUTO_EXPOSURE_INOUT_DATA>();
		cBuffer.WriteData(&data);
		pd3dCommandList->SetComputeRootConstantBufferView(rootParamCBData, cBuffer.GPUAddress);

		pd3dCommandList->Dispatch(
			CeilDiv(unTargetWidths.back(), FINAL_THREAD_X),
			1,
			1
		);
	}
}

void AutoExposurePass::OnPostRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle)
{
	// Set Bloom Result
	const auto pFinalLuminance = RENDER->GetPostProcessingResources().LuminanceFinalBuffer.GetResource();
	pFinalLuminance->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);		// Final output

	const uint32 unDescriptorInc = D3DCore::GetDescriptorIncrementSize(DESCRIPTOR_TYPE::CBV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle = outDescHandle.cpuHandle;
	constexpr uint32 rootParamLuminance = std::to_underlying(ROOT_PARAMETER::LUMINANCE);

	DEVICE->CopyDescriptorsSimple(1, cpuHandle, pFinalLuminance->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	pd3dCommandList->SetGraphicsRootDescriptorTable(rootParamLuminance, outDescHandle.gpuHandle);
	outDescHandle.cpuHandle.Offset(1, unDescriptorInc);
	outDescHandle.gpuHandle.Offset(1, unDescriptorInc);

	// Reset root signature
	RENDER->SetGlobalRootSignature(pd3dCommandList.Get());
}

void AutoExposurePass::CreatePipelineStates()
{
	HRESULT hr{};
	D3D12_COMPUTE_PIPELINE_STATE_DESC d3dComputePipelineDesc;

	// m_pBrightExtractPSO
	{
		d3dComputePipelineDesc.pRootSignature = RenderManager::g_pd3dComputeRootSignature.Get();
		d3dComputePipelineDesc.CS = SHADER->GetShaderByteCode("ExtractLuminanceCS");
		d3dComputePipelineDesc.CachedPSO = { nullptr, 0 };
		d3dComputePipelineDesc.NodeMask = 0;
		d3dComputePipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	}
	hr = DEVICE->CreateComputePipelineState(&d3dComputePipelineDesc, IID_PPV_ARGS(m_pd3dLuminanceExtractPipelineState.GetAddressOf()));

	// m_pBlurHPSO
	{
		d3dComputePipelineDesc.CS = SHADER->GetShaderByteCode("ReduceLuminanceCS");
	}
	hr = DEVICE->CreateComputePipelineState(&d3dComputePipelineDesc, IID_PPV_ARGS(m_pd3dReduceLuminancePipelineState.GetAddressOf()));

	// m_pBlurVPSO
	{
		d3dComputePipelineDesc.CS = SHADER->GetShaderByteCode("FinalLuminanceCS");
	}
	hr = DEVICE->CreateComputePipelineState(&d3dComputePipelineDesc, IID_PPV_ARGS(m_pd3dFinalPipelineState.GetAddressOf()));
}
