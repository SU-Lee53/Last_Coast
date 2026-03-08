#include "pch.h"
#include "ComputePass.h"

void IComputePass::Initialize()
{
	CreateRootSignature();
	CreatePipelineState();
}

void HDRIToCubeMapPass::CreateRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE d3dDescriptorRanges[2];
	d3dDescriptorRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, 0);
	d3dDescriptorRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);

	CD3DX12_ROOT_PARAMETER d3dRootParameters[2];
	d3dRootParameters[0].InitAsDescriptorTable(2, d3dDescriptorRanges, D3D12_SHADER_VISIBILITY_ALL);
	d3dRootParameters[1].InitAsConstants(sizeof(CB_SKYBOX_SIZE) / sizeof(int), 0, 0, D3D12_SHADER_VISIBILITY_ALL);

	D3D12_ROOT_SIGNATURE_FLAGS d3dRootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;

	D3D12_STATIC_SAMPLER_DESC d3dSamplerDescs[1];
	d3dSamplerDescs[0].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	d3dSamplerDescs[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	d3dSamplerDescs[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	d3dSamplerDescs[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	d3dSamplerDescs[0].MipLODBias = 0;
	d3dSamplerDescs[0].MaxAnisotropy = 1;
	d3dSamplerDescs[0].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	d3dSamplerDescs[0].MinLOD = 0;
	d3dSamplerDescs[0].MaxLOD = D3D12_FLOAT32_MAX;
	d3dSamplerDescs[0].ShaderRegister = 0;
	d3dSamplerDescs[0].RegisterSpace = 0;
	d3dSamplerDescs[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_ROOT_SIGNATURE_DESC d3dRootSignatureDesc{};
	{
		d3dRootSignatureDesc.NumParameters = _countof(d3dRootParameters);
		d3dRootSignatureDesc.pParameters = d3dRootParameters;
		d3dRootSignatureDesc.NumStaticSamplers = _countof(d3dSamplerDescs);
		d3dRootSignatureDesc.pStaticSamplers = d3dSamplerDescs;
		d3dRootSignatureDesc.Flags = d3dRootSignatureFlags;
	}

	ComPtr<ID3DBlob> pd3dSignatureBlob = nullptr;
	ComPtr<ID3DBlob> pd3dErrorBlob = nullptr;

	HRESULT hr = D3D12SerializeRootSignature(&d3dRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, pd3dSignatureBlob.GetAddressOf(), pd3dErrorBlob.GetAddressOf());
	if (FAILED(hr)) {
		char* pErrorString = (char*)pd3dErrorBlob->GetBufferPointer();
		HWND hWnd = ::GetActiveWindow();
		MessageBoxA(hWnd, pErrorString, NULL, 0);
		OutputDebugStringA(pErrorString);
		__debugbreak();
	}

	DEVICE->CreateRootSignature(0, pd3dSignatureBlob->GetBufferPointer(), pd3dSignatureBlob->GetBufferSize(), IID_PPV_ARGS(m_pd3dRootSignature.GetAddressOf()));
}

void HDRIToCubeMapPass::CreatePipelineState()
{
	D3D12_COMPUTE_PIPELINE_STATE_DESC d3dPipelineDesc{};
	{
		d3dPipelineDesc.pRootSignature = m_pd3dRootSignature.Get();
		d3dPipelineDesc.CS = SHADER->GetShaderByteCode("HDRIToCubeMapCS");
		d3dPipelineDesc.NodeMask = 0;
		d3dPipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
		d3dPipelineDesc.CachedPSO = D3D12_CACHED_PIPELINE_STATE{nullptr, 0};
	}
	DEVICE->CreateComputePipelineState(&d3dPipelineDesc, IID_PPV_ARGS(m_pd3dPipelineState.GetAddressOf()));
}

void HDRIToCubeMapPass::Dispatch(const ComputePassInput& input, uint32 unNumThreadX, uint32 unNumThreadY, uint32 unNumThreadZ)
{
	auto& cmdPair = COMPUTE->AllocCommandList();
	ComPtr<ID3D12GraphicsCommandList> pd3dCommandList = cmdPair.pd3dCommandList;

	COMPUTE->SetDescriptorHeap(pd3dCommandList);
	pd3dCommandList->SetComputeRootSignature(m_pd3dRootSignature.Get());

	auto desc1 = COMPUTE->AllocDescriptor();
	CD3DX12_CPU_DESCRIPTOR_HANDLE hdriHandle = input.pSRVs[0]->GetSRVHandle();
	DEVICE->CopyDescriptorsSimple(1, desc1.cpuHandle, hdriHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	auto desc2 = COMPUTE->AllocDescriptor();
	CD3DX12_CPU_DESCRIPTOR_HANDLE cubemapHandle = input.pUAVs[0]->GetUAVHandle();
	DEVICE->CopyDescriptorsSimple(1, desc2.cpuHandle, cubemapHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pd3dCommandList->SetComputeRootDescriptorTable(0, desc1.gpuHandle);
	pd3dCommandList->SetComputeRoot32BitConstants(1, sizeof(CB_SKYBOX_SIZE) / sizeof(int), input.pAdditionalContext, 0);

	pd3dCommandList->SetPipelineState(m_pd3dPipelineState.Get());
	pd3dCommandList->Dispatch(unNumThreadX, unNumThreadY, unNumThreadZ);

	input.pUAVs[0]->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

	COMPUTE->IndirectExecute(pd3dCommandList, unNumThreadX, unNumThreadY, unNumThreadZ);
}
