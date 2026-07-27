#include "pch.h"
#include "RenderManager.h"
#include "MeshRenderer.h"
#include "TerrainObject.h"
#include "Skybox.h"
#include "RenderGraph.h"
#include "Scene.h"
#include "Light.h"

ComPtr<ID3D12RootSignature> RenderManager::g_pd3dGlobalRootSignature = nullptr;
ComPtr<ID3D12RootSignature> RenderManager::g_pd3dComputeRootSignature = nullptr;

void GBuffer::Initialize()
{
	for (uint32 i = 0; i < g_unNumGBuffers; ++i) {
		GBuffers[i] = TEXTURE->LoadRenderTargetTexture(
			"GBuffer_" + std::to_string(i),
			WinCore::g_dwClientWidth,
			WinCore::g_dwClientHeight,
			DXGI_FORMAT_R8G8B8A8_UNORM,
			DXGI_FORMAT_R8G8B8A8_UNORM);
	}
}

void GBuffer::Reset()
{
	for (auto& buffer : GBuffers) {
		buffer = {};
	}
}

void PostProcessingResources::Initialize()
{
	const DWORD nWidth = WinCore::g_dwClientWidth;
	const DWORD nHeight = WinCore::g_dwClientHeight;
	for (uint32 i = 0; i < 2; ++i) {
		BloomHalfBuffer[i] = TEXTURE->LoadRWRenderTargetTexture(
			std::format("Bloom2_{}", i),
			nWidth / 2,
			nHeight / 2,
			DXGI_FORMAT_R16G16B16A16_FLOAT,
			DXGI_FORMAT_R16G16B16A16_FLOAT,
			DXGI_FORMAT_R16G16B16A16_FLOAT
		);

		BloomQuaterBuffer[i] = TEXTURE->LoadRWRenderTargetTexture(
			std::format("Bloom4_{}", i),
			nWidth / 4,
			nHeight / 4,
			DXGI_FORMAT_R16G16B16A16_FLOAT,
			DXGI_FORMAT_R16G16B16A16_FLOAT,
			DXGI_FORMAT_R16G16B16A16_FLOAT
		);

		BloomEighthBuffer[i] = TEXTURE->LoadRWRenderTargetTexture(
			std::format("Bloom8_{}", i),
			nWidth / 8,
			nHeight / 8,
			DXGI_FORMAT_R16G16B16A16_FLOAT,
			DXGI_FORMAT_R16G16B16A16_FLOAT,
			DXGI_FORMAT_R16G16B16A16_FLOAT
		);
	}

	// SSAO
	{
		SSAOBuffer = TEXTURE->LoadRenderTargetTexture(
			"SSAO",
			nWidth,
			nHeight,
			DXGI_FORMAT_R16_FLOAT,
			DXGI_FORMAT_R16_FLOAT
		);

		SSAOBlurBuffer = TEXTURE->LoadRenderTargetTexture(
			"SSAO_Blur",
			nWidth,
			nHeight,
			DXGI_FORMAT_R16_FLOAT,
			DXGI_FORMAT_R16_FLOAT
		);

		LightShaftBuffer = TEXTURE->LoadRenderTargetTexture(
			"LightShaft",
			nWidth,
			nHeight,
			DXGI_FORMAT_R16G16B16A16_FLOAT,
			DXGI_FORMAT_R16G16B16A16_FLOAT
		);
	}
	
	// Luminance
	{
		for (int i = 0; i < LuminanceBuffer.size(); ++i) {
			int32 nSizeDiv = (i + 1) * 2;
			LuminanceBuffer[i] = TEXTURE->LoadRWRenderTargetTexture(
				"Luminance_" + std::to_string(nSizeDiv),
				nWidth / nSizeDiv,
				nHeight / nSizeDiv,
				DXGI_FORMAT_R16_FLOAT,
				DXGI_FORMAT_R16_FLOAT
			);
		}
		
		LuminanceFinalBuffer = TEXTURE->LoadRWRenderTargetTexture(
			"Luminance_Final",
			1,
			1,
			DXGI_FORMAT_R16_FLOAT,
			DXGI_FORMAT_R16_FLOAT
		);
	}

}

void PostProcessingResources::Reset()
{
	SSAOBuffer = {};
	SSAOBlurBuffer = {};
	LightShaftBuffer = {};

	for (auto& buffer : BloomHalfBuffer)
		buffer = {};
	for (auto& buffer : BloomQuaterBuffer)
		buffer = {};
	for (auto& buffer : BloomEighthBuffer)
		buffer = {};
	for (auto& buffer : LuminanceBuffer)
		buffer = {};

	LuminanceFinalBuffer = {};
}

void RenderManager::Initialize(ComPtr<ID3D12Device> pd3dDevice)
{
	m_pd3dDevice = pd3dDevice;
	CreateFence();
	CreateCommandQueueAndList();
	CreateSwapChain();
	CreateRenderTarget();
	CreateDepthStencil();

	CreateGlobalRootSignature(pd3dDevice);
	CreateComputeRootSignature(pd3dDevice);

	// DescriptorHandle Heap For Draw
	D3D12_DESCRIPTOR_HEAP_DESC d3dHeapDesc;
	{
		d3dHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		d3dHeapDesc.NumDescriptors = DESCRIPTOR_PER_DRAW;
		d3dHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		d3dHeapDesc.NodeMask = 0;
	}

	for (uint32 i = 0; i < g_unMaxPendingFrames; ++i) {
		m_DescriptorHeapForDraw[i].Initialize(pd3dDevice, d3dHeapDesc);
		m_ConstantBufferPool[i].Initialize(24000);
		m_StructuredBufferPool[i].Initialize(18'000'000, 3600);
	}


	CreateResolutionDependentResources();

	m_pQuadMesh = std::make_shared<QuadMesh>(-1, 1);

	m_TextRenderer.Initialize(m_pd3dCommandQueue);
}

bool RenderManager::Resize(uint32 unWidth, uint32 unHeight)
{
	if (!m_pdxgiSwapChain || unWidth == 0 || unHeight == 0)
		return false;

	if (WinCore::g_dwClientWidth == unWidth &&
		WinCore::g_dwClientHeight == unHeight)
		return true;

	WaitForGPUComplete();
	ReleaseResolutionDependentResources();

	UINT unSwapChainFlags = m_bAllowTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
	HRESULT hr = m_pdxgiSwapChain->ResizeBuffers(
		g_unMaxPendingFrames,
		unWidth,
		unHeight,
		DXGI_FORMAT_UNKNOWN,
		unSwapChainFlags
	);

	if (FAILED(hr)) {
		CreateRenderTarget();
		CreateDepthStencil();
		CreateResolutionDependentResources();
		SHOW_ERROR("Failed to resize SwapChain");
		return false;
	}

	WinCore::g_dwClientWidth = unWidth;
	WinCore::g_dwClientHeight = unHeight;
	m_unBackBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();

	CreateRenderTarget();
	CreateDepthStencil();
	CreateResolutionDependentResources();

	return true;
}

void RenderManager::CreateGlobalRootSignature(ComPtr<ID3D12Device> pd3dDevice)
{
	CD3DX12_DESCRIPTOR_RANGE d3dDescriptorRanges[29]; 
	// space0 : Per Scene (Frame) 
	d3dDescriptorRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, 0); // cbSceneData 
	d3dDescriptorRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND); // gLightData 

	// space0 : Cascade shadow maps
	d3dDescriptorRanges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1, 0, 0);					// cbCascadeShadowMatrix
	d3dDescriptorRanges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 1, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND); // gtxtCascadeShadowMaps[NUM_CASCADES]
	
	// space0 : Shadow maps
	d3dDescriptorRanges[4].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2, 0, 0); // cbShadowMatrix
	d3dDescriptorRanges[5].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 5, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND); // gtxtShadows[4]

	// space0 : GBuffer[3] + Depth[1]
	d3dDescriptorRanges[6].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 9, 0, 0);
	d3dDescriptorRanges[7].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 12, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);

	// space0 : HDR Result
	d3dDescriptorRanges[8].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 13, 0, 0);

	// space0 : Bloom Result
	d3dDescriptorRanges[9].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 14, 0, 0);

	// space0 : SSAO
	d3dDescriptorRanges[10].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 6, 0, 0);	// SSAO data
	d3dDescriptorRanges[11].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 17, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);	// Noise
	d3dDescriptorRanges[12].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 18, 0, 0);	// SSAO
	d3dDescriptorRanges[13].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 19, 0, 0);	// SSAO blur

	// space0 : Luminance
	d3dDescriptorRanges[14].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 20, 0, 0);	// Luminance 

	// space0 : Light Shaft
	d3dDescriptorRanges[15].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 7, 0, 0);	// Light shaft data
	d3dDescriptorRanges[16].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 23, 0, 0);	// Light shaft result

	// space0 : ToneMapping
	d3dDescriptorRanges[17].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 2, 4, 0, 0);
	d3dDescriptorRanges[18].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 21, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
	
	// space1 : Per Pass 
	d3dDescriptorRanges[19].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1,		0, 1, 0); // gWorldTransforms
	d3dDescriptorRanges[20].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1,		1, 1, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND); // gBoneTransforms
	d3dDescriptorRanges[21].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1,		2, 1, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND); // gMaterialDatas
	d3dDescriptorRanges[22].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, UINT_MAX, 3, 1, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND); // gtxtTextures

	// space2 : cbTerrainLayerData
	d3dDescriptorRanges[23].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1, 2, 0); // cbTerrainLayerData
	d3dDescriptorRanges[24].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 3, 2, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND); // gtxtTerrainAlbedo[4] 
	d3dDescriptorRanges[25].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 7, 2, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND); // gtxtTerrainNormal[4]

	// space2 : Terrain component instancing data
	d3dDescriptorRanges[26].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 14, 2, 0); // gTerrainComponentData
	d3dDescriptorRanges[27].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 15, 2, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND); // gtxtTerrainHeightMap
	d3dDescriptorRanges[28].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, g_unMaxTerrainComponents, 16, 2, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND); // gtxtTerrainWeightMaps

	CD3DX12_ROOT_PARAMETER d3dRootParameters[24];
	// Per Scene
	d3dRootParameters[0].InitAsDescriptorTable(2, &d3dDescriptorRanges[0], D3D12_SHADER_VISIBILITY_ALL);	// Per Draw
	d3dRootParameters[1].InitAsDescriptorTable(2, &d3dDescriptorRanges[2], D3D12_SHADER_VISIBILITY_ALL);	// Cascade Shadow maps
	d3dRootParameters[2].InitAsDescriptorTable(2, &d3dDescriptorRanges[4], D3D12_SHADER_VISIBILITY_ALL);	// Shadow maps
	d3dRootParameters[3].InitAsDescriptorTable(2, &d3dDescriptorRanges[6], D3D12_SHADER_VISIBILITY_ALL);	// G-Buffers
	d3dRootParameters[4].InitAsDescriptorTable(1, &d3dDescriptorRanges[8], D3D12_SHADER_VISIBILITY_ALL);	// HDR Result
	d3dRootParameters[5].InitAsDescriptorTable(1, &d3dDescriptorRanges[9], D3D12_SHADER_VISIBILITY_ALL);	// Bloom Result
	d3dRootParameters[6].InitAsDescriptorTable(2, &d3dDescriptorRanges[10], D3D12_SHADER_VISIBILITY_ALL);	// SSAO Data + Noise
	d3dRootParameters[7].InitAsDescriptorTable(1, &d3dDescriptorRanges[12], D3D12_SHADER_VISIBILITY_ALL);	// SSAO In
	d3dRootParameters[8].InitAsDescriptorTable(1, &d3dDescriptorRanges[13], D3D12_SHADER_VISIBILITY_ALL);	// SSAO Out (Blur)
	d3dRootParameters[9].InitAsDescriptorTable(1, &d3dDescriptorRanges[14], D3D12_SHADER_VISIBILITY_ALL);	// Luminance
	d3dRootParameters[10].InitAsDescriptorTable(1, &d3dDescriptorRanges[15], D3D12_SHADER_VISIBILITY_ALL);	// Light Shaft Data
	d3dRootParameters[11].InitAsDescriptorTable(1, &d3dDescriptorRanges[16], D3D12_SHADER_VISIBILITY_ALL);	// Light Shaft Result
	d3dRootParameters[12].InitAsDescriptorTable(2, &d3dDescriptorRanges[17], D3D12_SHADER_VISIBILITY_ALL);	// Tone Mapping
	d3dRootParameters[13].InitAsConstantBufferView(3, 0, D3D12_SHADER_VISIBILITY_ALL);	// Fog parameters
	
	// Per Pass
	d3dRootParameters[14].InitAsDescriptorTable(3, &d3dDescriptorRanges[19], D3D12_SHADER_VISIBILITY_ALL);	// Per Pass
	d3dRootParameters[15].InitAsDescriptorTable(1, &d3dDescriptorRanges[22], D3D12_SHADER_VISIBILITY_ALL);	// Per Pass textures

	// Per Instance(Draw)
	d3dRootParameters[16].InitAsConstantBufferView(0, 2, D3D12_SHADER_VISIBILITY_ALL);		// cbInstanceData
	d3dRootParameters[17].InitAsShaderResourceView(2, 2, D3D12_SHADER_VISIBILITY_ALL);		// gBoneTransformOffsets
	d3dRootParameters[18].InitAsConstantBufferView(3, 2, D3D12_SHADER_VISIBILITY_ALL);		// cbLightCameraData

	d3dRootParameters[19].InitAsDescriptorTable(3, &d3dDescriptorRanges[23], D3D12_SHADER_VISIBILITY_ALL);	// TerrainLayer
	d3dRootParameters[20].InitAsDescriptorTable(3, &d3dDescriptorRanges[26], D3D12_SHADER_VISIBILITY_ALL);	// TerrainComponentData + HeightMap + WeightMaps
	d3dRootParameters[21].InitAsConstantBufferView(4, 2, D3D12_SHADER_VISIBILITY_ALL);		// gnWorldTransformIndex

	d3dRootParameters[22].InitAsShaderResourceView(12, 2, D3D12_SHADER_VISIBILITY_ALL);		// gUIData
	d3dRootParameters[23].InitAsShaderResourceView(13, 2, D3D12_SHADER_VISIBILITY_ALL);		// gParticleData

	CD3DX12_STATIC_SAMPLER_DESC d3dSamplerDesc[6];
	// s0 : SkyboxSampler
	d3dSamplerDesc[0].Init(0);
	d3dSamplerDesc[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	d3dSamplerDesc[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	d3dSamplerDesc[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	d3dSamplerDesc[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	d3dSamplerDesc[0].MinLOD = 0;
	d3dSamplerDesc[0].MaxLOD = D3D12_FLOAT32_MAX;
	d3dSamplerDesc[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// s1 : WeightMap Sampler
	d3dSamplerDesc[1].Init(1);
	d3dSamplerDesc[1].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	d3dSamplerDesc[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	d3dSamplerDesc[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	d3dSamplerDesc[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	d3dSamplerDesc[1].MinLOD = 0;
	d3dSamplerDesc[1].MaxLOD = D3D12_FLOAT32_MAX;
	d3dSamplerDesc[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// s2 : TextureSampler (General)
	d3dSamplerDesc[2].Init(2);
	d3dSamplerDesc[2].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	d3dSamplerDesc[2].MinLOD = 0;
	d3dSamplerDesc[2].MaxLOD = D3D12_FLOAT32_MAX;
	d3dSamplerDesc[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// s3 : TextureSampler (General)
	d3dSamplerDesc[3].Init(3);
	d3dSamplerDesc[3].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	d3dSamplerDesc[4].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	d3dSamplerDesc[4].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	d3dSamplerDesc[4].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	d3dSamplerDesc[3].MinLOD = 0;
	d3dSamplerDesc[3].MaxLOD = D3D12_FLOAT32_MAX;
	d3dSamplerDesc[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// s4 : Anistropic : used for terrain
	d3dSamplerDesc[4].Init(4);
	d3dSamplerDesc[4].Filter = D3D12_FILTER_ANISOTROPIC;
	d3dSamplerDesc[4].MinLOD = 0;
	d3dSamplerDesc[4].MaxLOD = D3D12_FLOAT32_MAX;
	d3dSamplerDesc[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// s5 : ComparisonSampler
	d3dSamplerDesc[5].Init(5);
	d3dSamplerDesc[5].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	d3dSamplerDesc[5].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	d3dSamplerDesc[5].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	d3dSamplerDesc[5].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	d3dSamplerDesc[5].MipLODBias = 0.0f;
	d3dSamplerDesc[5].MaxAnisotropy = 1;
	d3dSamplerDesc[5].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	d3dSamplerDesc[5].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
	d3dSamplerDesc[5].MinLOD = 0;
	d3dSamplerDesc[5].MaxLOD = D3D12_FLOAT32_MAX;
	d3dSamplerDesc[5].ShaderRegister = 5;
	d3dSamplerDesc[5].RegisterSpace = 0;
	d3dSamplerDesc[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_FLAGS d3dRootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC d3dRootSignatureDesc{};
	d3dRootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_0;
	d3dRootSignatureDesc.Desc_1_0.NumParameters = _countof(d3dRootParameters);
	d3dRootSignatureDesc.Desc_1_0.pParameters = d3dRootParameters;
	d3dRootSignatureDesc.Desc_1_0.NumStaticSamplers = _countof(d3dSamplerDesc);
	d3dRootSignatureDesc.Desc_1_0.pStaticSamplers = d3dSamplerDesc;
	d3dRootSignatureDesc.Desc_1_0.Flags = d3dRootSignatureFlags;

	ComPtr<ID3DBlob> pd3dSignatureBlob = nullptr;
	ComPtr<ID3DBlob> pd3dErrorBlob = nullptr;

	HRESULT hr = D3D12SerializeVersionedRootSignature(&d3dRootSignatureDesc, pd3dSignatureBlob.GetAddressOf(), pd3dErrorBlob.GetAddressOf());
	if (FAILED(hr)) {
		char* pErrorString = (char*)pd3dErrorBlob->GetBufferPointer();
		HWND hWnd = ::GetActiveWindow();
		MessageBoxA(hWnd, pErrorString, NULL, 0);
		OutputDebugStringA(pErrorString);
		__debugbreak();
	}

	pd3dDevice->CreateRootSignature(
		0, 
		pd3dSignatureBlob->GetBufferPointer(), 
		pd3dSignatureBlob->GetBufferSize(), 
		IID_PPV_ARGS(g_pd3dGlobalRootSignature.GetAddressOf())
	);
}

void RenderManager::CreateComputeRootSignature(ComPtr<ID3D12Device> pd3dDevice)
{
	CD3DX12_DESCRIPTOR_RANGE d3dDescriptorRanges[4];
	// space0 : Input SRV, Output UAV
	d3dDescriptorRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, 0);
	d3dDescriptorRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
	
	d3dDescriptorRanges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0, 0);
	d3dDescriptorRanges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);


	CD3DX12_ROOT_PARAMETER d3dRootParameters[6];
	// space0 : Input/Output
	d3dRootParameters[0].InitAsDescriptorTable(1, &d3dDescriptorRanges[0], D3D12_SHADER_VISIBILITY_ALL);
	d3dRootParameters[1].InitAsDescriptorTable(1, &d3dDescriptorRanges[1], D3D12_SHADER_VISIBILITY_ALL);
	d3dRootParameters[2].InitAsDescriptorTable(1, &d3dDescriptorRanges[2], D3D12_SHADER_VISIBILITY_ALL);
	d3dRootParameters[3].InitAsDescriptorTable(1, &d3dDescriptorRanges[3], D3D12_SHADER_VISIBILITY_ALL);

	// space1 : buffers
	d3dRootParameters[4].InitAsConstantBufferView(0, 1, D3D12_SHADER_VISIBILITY_ALL);
	d3dRootParameters[5].InitAsConstantBufferView(1, 1, D3D12_SHADER_VISIBILITY_ALL);


	CD3DX12_STATIC_SAMPLER_DESC d3dSamplerDesc[1];
	// s0 : linear
	d3dSamplerDesc[0].Init(0);
	d3dSamplerDesc[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	d3dSamplerDesc[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	d3dSamplerDesc[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	d3dSamplerDesc[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	d3dSamplerDesc[0].MinLOD = 0;
	d3dSamplerDesc[0].MaxLOD = D3D12_FLOAT32_MAX;
	d3dSamplerDesc[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_ROOT_SIGNATURE_FLAGS d3dRootSignatureFlags = 
		D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC d3dRootSignatureDesc{};
	d3dRootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_0;
	d3dRootSignatureDesc.Desc_1_0.NumParameters = _countof(d3dRootParameters);
	d3dRootSignatureDesc.Desc_1_0.pParameters = d3dRootParameters;
	d3dRootSignatureDesc.Desc_1_0.NumStaticSamplers = _countof(d3dSamplerDesc);
	d3dRootSignatureDesc.Desc_1_0.pStaticSamplers = d3dSamplerDesc;
	d3dRootSignatureDesc.Desc_1_0.Flags = d3dRootSignatureFlags;

	ComPtr<ID3DBlob> pd3dSignatureBlob = nullptr;
	ComPtr<ID3DBlob> pd3dErrorBlob = nullptr;

	HRESULT hr = D3D12SerializeVersionedRootSignature(&d3dRootSignatureDesc, pd3dSignatureBlob.GetAddressOf(), pd3dErrorBlob.GetAddressOf());
	if (FAILED(hr)) {
		char* pErrorString = (char*)pd3dErrorBlob->GetBufferPointer();
		HWND hWnd = ::GetActiveWindow();
		MessageBoxA(hWnd, pErrorString, NULL, 0);
		OutputDebugStringA(pErrorString);
		__debugbreak();
	}

	pd3dDevice->CreateRootSignature(
		0,
		pd3dSignatureBlob->GetBufferPointer(),
		pd3dSignatureBlob->GetBufferSize(),
		IID_PPV_ARGS(g_pd3dComputeRootSignature.GetAddressOf())
	);
}

void RenderManager::SetGlobalRootSignature(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList)
{
	pd3dCommandList->SetGraphicsRootSignature(g_pd3dGlobalRootSignature.Get());
}

void RenderManager::SetComputeRootSignature(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList)
{
	pd3dCommandList->SetComputeRootSignature(g_pd3dComputeRootSignature.Get());
}

void RenderManager::BuildRenderGraph()
{
	m_RenderGraph.BuildGraph();
}

void RenderManager::Render()
{
	OnPrepareRender();

	ComPtr<ID3D12GraphicsCommandList> pd3dCommandList = m_ppd3dCommandList[m_unCurrentContextIndex];

	pd3dCommandList->SetGraphicsRootSignature(g_pd3dGlobalRootSignature.Get());
	pd3dCommandList->SetDescriptorHeaps(1, m_DescriptorHeapForDraw[m_unCurrentContextIndex].GetD3DDescriptorHeap().GetAddressOf());
	
	auto pCamera = CUR_SCENE->GetCamera();
	pCamera->SetViewportsAndScissorRects(pd3dCommandList);

	DescriptorHandle descHandle = m_DescriptorHeapForDraw[m_unCurrentContextIndex].GetDescriptorHandleFromHeapStart();
	BindPerSceneData(pd3dCommandList, descHandle);

	RenderPassInput input{};
	m_RenderGraph.Run(descHandle, pd3dCommandList, input);

	GUI->Render(pd3dCommandList);

	OnPostRender();
	Present();
}

void RenderManager::ShowDebugOptions()
{
	ImGui::Begin("RenderManager");

	ImGui::Text("Current Context : %d", m_unCurrentContextIndex);

	ImGui::Text("IGameObject in current scene : %d", CUR_SCENE->GetWorld().SizeAll());
	ImGui::Text("IGameObject Render Candidates : %d", m_pObjectsToRender.size());

	ImGui::SeparatorText("Render Queue");
	ImGui::Text("Opaque : %d", m_pObjectsToRender.size());
	ImGui::Text("Transparent: %d", m_pTransparentObjectsToRender.size());
	//for (uint32 i = 0; i < g_unMaxSpriteLayers; ++i) {
	//	ImGui::Text("Sprite Layer[%d] : %d", i, m_pSpritesToRender[i].size());
	//}

	ImGui::SeparatorText("Render Graph");
	m_RenderGraph.ShowDebugInfo();

	ImGui::SeparatorText("Frame Resources");
	{
		if (ImGui::TreeNode("Descriptor Heaps")) {
			ImGui::Text("DescriptorSize = %d", m_DescriptorHeapForDraw->m_uiDescriptorSize);
			ImGui::Text("CurrentDescriptorCount = %d", m_DescriptorHeapForDraw->m_uiCurrentDescriptorCount);
			ImGui::TreePop();
		}

		if (ImGui::TreeNode("ConstantBufferPool")) {
			m_ConstantBufferPool->ShowDebugInfo();
			ImGui::TreePop();
		}

		if (ImGui::TreeNode("StructuredBufferPool")) {
			m_StructuredBufferPool->ShowDebugInfo();
			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Render Targets")) {
			if (ImGui::TreeNode("Back Buffer")) {
				auto pBackBuffer = m_BackBufferIDs[0].GetResource();
				pBackBuffer->ShowDebugInfo();

				ImGui::TreePop();
			}
			
			if (ImGui::TreeNode("Main Depth Buffer")) {
				auto pDepthBuffer = m_DepthStencilID.GetResource();
				pDepthBuffer->ShowDebugInfo();

				ImGui::TreePop();
			}
			
			if (ImGui::TreeNode("G-Buffers")) {
				ImGui::Text("GBuffer[0][0] Info. Rest is same");
				auto pRTV = m_GBuffers.GBuffers[0].GetResource();
				pRTV->ShowDebugInfo();

				ImGui::TreePop();
			}
			
			if (ImGui::TreeNode("HDR0")) {
				ImGui::Text("HDR[0] Info. Rest is same");
				auto pHDR = m_HDRRenderTargetIDs[0].GetResource();
				pHDR->ShowDebugInfo();

				ImGui::TreePop();
			}
			
			if (ImGui::TreeNode("HDR1")) {
				ImGui::Text("HDR[1] Info. Rest is same");
				auto pHDR = m_HDRRenderTargetIDs[1].GetResource();
				pHDR->ShowDebugInfo();

				ImGui::TreePop();
			}
			
			if (ImGui::TreeNode("LDR")) {
				ImGui::Text("LDR Info.");
				auto pLDR = m_LDRRenderTargetIDs.GetResource();
				pLDR->ShowDebugInfo();

				ImGui::TreePop();
			}

			ImGui::TreePop();
		}

	}


	ImGui::End();
}

void RenderManager::Reset()
{
	m_pObjectsToRender.clear();
	m_pTransparentObjectsToRender.clear();

	m_ConstantBufferPool[m_unCurrentContextIndex].Reset();
	m_StructuredBufferPool[m_unCurrentContextIndex].Reset();
}

void RenderManager::BindPerSceneData(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, OUT DescriptorHandle& outDescHandle)
{
	// descRange[0]  :  cbSceneData					1
	// descRange[1]  :  gLightData					1
	uint32 unDescIncrementSize = D3DCore::g_nCBVSRVDescriptorIncrementSize;

	// Cache
	const auto& pTerrain = CUR_SCENE->GetTerrain();
	const auto& pSkybox = CUR_SCENE->GetSkybox();
	const auto& pCamera = CUR_SCENE->GetCamera();

	std::vector<LightData> lightDatas;
	lightDatas.reserve(MAX_LIGHTS);

	auto AppendLightData = [&lightDatas](Light* pLight)
		{
			if (!pLight || !pLight->m_bEnable || lightDatas.size() >= MAX_LIGHTS) {
				return;
			}

			lightDatas.push_back(pLight->MakeCBData());
		};

	for (const auto& pLight : CUR_SCENE->GetLightsInScene()) {
		if (dynamic_cast<DirectionalLight*>(pLight.get())) {
			AppendLightData(pLight.get());
		}
	}

	SpatialQueryDesc lightQueryDesc{};
	lightQueryDesc.unLayerMask = SPATIAL_LIGHT;
	lightQueryDesc.eLayerMatchMode = SPATIAL_LAYER_MATCH_MODE::ALL;
	lightQueryDesc.bIncludeStatic = true;
	lightQueryDesc.bIncludeDynamic = true;

	SpatialQueryResult visibleLightResult = CUR_SCENE->GetWorld().GetSpatial().QueryFrustum(
		pCamera->GetFrustumWorld(),
		lightQueryDesc
	);

	if (visibleLightResult.pLights.size() > MAX_LIGHTS - lightDatas.size()) {
		const Vector3 v3CameraPos = pCamera->GetPosition();
		std::sort(visibleLightResult.pLights.begin(), visibleLightResult.pLights.end(),
			[&v3CameraPos](const Light* pLhs, const Light* pRhs)
			{
				const float fLhsDistanceToInfluence = std::max(0.0f, Vector3::Distance(v3CameraPos, pLhs->m_v3Position) - pLhs->m_fRange);
				const float fRhsDistanceToInfluence = std::max(0.0f, Vector3::Distance(v3CameraPos, pRhs->m_v3Position) - pRhs->m_fRange);
				return fLhsDistanceToInfluence < fRhsDistanceToInfluence;
			});
	}

	for (Light* pLight : visibleLightResult.pLights) {
		if (dynamic_cast<DirectionalLight*>(pLight)) {
			continue;
		}

		AppendLightData(pLight);
	}

	// descRange[0]
	CB_SCENE_DATA sceneCBData;
	{
		sceneCBData.gSceneGlobal.fTotalTime = TIME->GetTotalTime();
		sceneCBData.gSceneGlobal.fElapsedTime = TIME->GetTimeElapsed();
		sceneCBData.gSceneGlobal.nNumLights = static_cast<int32>(lightDatas.size());
		sceneCBData.gSceneGlobal.v4GlobalAmbient = CUR_SCENE->GetGlobalAmbient();
		sceneCBData.gCamera = pCamera->MakeCBData();
		sceneCBData.gSkybox = (pSkybox) ? CUR_SCENE->GetSkybox()->MakeCBData() : SkyboxData{};
		sceneCBData.nScreenSize = XMINT2{ 
			static_cast<int32>(WinCore::g_dwClientWidth), 
			static_cast<int32>(WinCore::g_dwClientHeight)
		};
	}
	auto perSceneCBuffer = AllocCBuffer<CB_SCENE_DATA>();
	perSceneCBuffer.WriteData(&sceneCBData);
	DEVICE->CopyDescriptorsSimple(1, outDescHandle.cpuHandle, perSceneCBuffer.CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	outDescHandle.cpuHandle.Offset(1, unDescIncrementSize);

	// descRange[1]
	auto lightSBuffer = AllocSBuffer<LightData>(lightDatas.size());
	lightSBuffer.WriteData(lightDatas);
	DEVICE->CopyDescriptorsSimple(1, outDescHandle.cpuHandle, lightSBuffer.SRVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	outDescHandle.cpuHandle.Offset(1, unDescIncrementSize);

	pd3dCommandList->SetGraphicsRootDescriptorTable(std::to_underlying(ROOT_PARAMETER::PER_SCENE_DATA), outDescHandle.gpuHandle);
	outDescHandle.gpuHandle.Offset(2, unDescIncrementSize);	// 1 + 1

}

const TextureRef<RenderTargetTexture>& RenderManager::GetCurrentBackBuffer() const
{
	return m_BackBufferIDs[m_unBackBufferIndex];
}

const TextureRef<DepthStencilTexture>& RenderManager::GetDepthStencilBuffer() const
{
	return m_DepthStencilID;
}

const CD3DX12_CPU_DESCRIPTOR_HANDLE RenderManager::GetCurrentBackBufferHandle() const
{
	return static_pointer_cast<RenderTargetTexture>(m_BackBufferIDs[m_unBackBufferIndex].GetResource())->GetRTVHandle();
}

const CD3DX12_CPU_DESCRIPTOR_HANDLE RenderManager::GetDepthStencilBufferHandle() const
{
	return static_pointer_cast<DepthStencilTexture>(m_DepthStencilID.GetResource())->GetDSVHandle();
}

const GBuffer& RenderManager::GetGBuffer() const
{
	return m_GBuffers;
}

const PostProcessingResources& RenderManager::GetPostProcessingResources() const
{
	return m_PostProcessingResources;
}

const TextureRef<RenderTargetTexture>& RenderManager::GetHDRBuffer(int nIndex) const
{
	return m_HDRRenderTargetIDs[nIndex];
}

const TextureRef<RenderTargetTexture>& RenderManager::GetLDRBuffer() const
{
	return m_LDRRenderTargetIDs;
}

const CD3DX12_CPU_DESCRIPTOR_HANDLE RenderManager::GetHDRBufferHandle(int nIndex) const
{
	return static_pointer_cast<RenderTargetTexture>(m_HDRRenderTargetIDs[nIndex].GetResource())->GetRTVHandle();
}

const CD3DX12_CPU_DESCRIPTOR_HANDLE RenderManager::GetLDRBufferHandle() const
{
	return static_pointer_cast<RenderTargetTexture>(m_LDRRenderTargetIDs.GetResource())->GetRTVHandle();
}

void RenderManager::OnPrepareRender()
{
	ComPtr<ID3D12CommandAllocator> pd3dCommandAllocator = m_ppd3dCommandAllocator[m_unCurrentContextIndex];
	ComPtr<ID3D12GraphicsCommandList> pd3dCommandList = m_ppd3dCommandList[m_unCurrentContextIndex];
	HRESULT hr{};

	hr = pd3dCommandAllocator->Reset();
	if (FAILED(hr)) {
		SHOW_ERROR("Faied to reset CommandAllocator");
		__debugbreak();
	}

	hr = pd3dCommandList->Reset(pd3dCommandAllocator.Get(), NULL);
	if (FAILED(hr)) {
		SHOW_ERROR("Faied to reset CommandList");
		__debugbreak();
	}

	GetCurrentBackBuffer().GetResource()->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
	GetDepthStencilBuffer().GetResource()->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_DEPTH_WRITE);

	CD3DX12_CPU_DESCRIPTOR_HANDLE d3dRTVCPUDescriptorHandle = GetCurrentBackBufferHandle();

	const auto& dsvRef = RENDER->GetDepthStencilBuffer();
	auto pDSV = dsvRef.GetResource();
	CD3DX12_CPU_DESCRIPTOR_HANDLE d3dDSVDescriptorHandle = pDSV->GetDSVHandle();

	float pfClearColor[4] = { 0.f, 0.0f, 0.0f, 1.0f };
	pd3dCommandList->ClearRenderTargetView(d3dRTVCPUDescriptorHandle, pfClearColor, 0, NULL);
	pd3dCommandList->ClearDepthStencilView(d3dDSVDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, NULL);
	
	pd3dCommandList->OMSetRenderTargets(1, &d3dRTVCPUDescriptorHandle, TRUE, &d3dDSVDescriptorHandle);
}

void RenderManager::OnPostRender()
{
	ComPtr<ID3D12GraphicsCommandList> pd3dCommandList = m_ppd3dCommandList[m_unCurrentContextIndex];
	HRESULT hr;

	// Change rendered render target's resource state from D3D12_RESOURCE_STATE_RENDER_TARGET to D3D12_RESOURCE_STATE_PRESENT
	auto pBackBuffer = GetCurrentBackBuffer().GetResource();
	pBackBuffer->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_PRESENT);
	pd3dCommandList->Close();

	ID3D12CommandList* ppd3dCommandLists[] = { pd3dCommandList.Get() };
	m_pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists);
}

void RenderManager::CreateFence()
{
	m_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_pd3dFence.GetAddressOf()));
	m_hFenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
}

void RenderManager::CreateCommandQueueAndList()
{
	HRESULT hr{};

	D3D12_COMMAND_QUEUE_DESC d3dCommandQueueDesc{};
	{
		d3dCommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		d3dCommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	}

	hr = m_pd3dDevice->CreateCommandQueue(&d3dCommandQueueDesc, IID_PPV_ARGS(m_pd3dCommandQueue.GetAddressOf()));
	if (FAILED(hr)) {
		SHOW_ERROR("Failed to create CommandQueue");
	}

	for (uint32 i = 0; i < g_unMaxPendingFrames; ++i) {
		// Create Command Allocator
		hr = m_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_ppd3dCommandAllocator[i].GetAddressOf()));
		if (FAILED(hr)) {
			SHOW_ERROR("Failed to create CommandAllocator");
		}

		// Create Command List
		hr = m_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_ppd3dCommandAllocator[i].Get(), NULL, IID_PPV_ARGS(m_ppd3dCommandList[i].GetAddressOf()));
		if (FAILED(hr)) {
			SHOW_ERROR("Failed to create CommandList");
		}
		// Close Command List(default is opened)
		hr = m_ppd3dCommandList[i]->Close();

	}

	m_ImmediateTransitionCmdLists.Initialize(30);
}

void RenderManager::CreateSwapChain()
{
	ComPtr<IDXGIFactory4> pFactory = DXGI_FACTORY;
	ComPtr<IDXGIFactory5> pFactory5;

	BOOL bAllowTearing = FALSE;
	if (SUCCEEDED(pFactory.As(&pFactory5))) {
		HRESULT hr = pFactory5->CheckFeatureSupport(
			DXGI_FEATURE_PRESENT_ALLOW_TEARING,
			&bAllowTearing,
			sizeof(bAllowTearing)
		);
		m_bAllowTearing = SUCCEEDED(hr) && bAllowTearing;
	}

	DXGI_SWAP_CHAIN_DESC1 dxgiSwapChainDesc{};
	{
		dxgiSwapChainDesc.Width = WinCore::g_dwClientWidth;
		dxgiSwapChainDesc.Height = WinCore::g_dwClientHeight;
		dxgiSwapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		dxgiSwapChainDesc.Stereo = FALSE;
		dxgiSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		dxgiSwapChainDesc.BufferCount = g_unMaxPendingFrames;
		dxgiSwapChainDesc.SampleDesc.Count = 1;
		dxgiSwapChainDesc.SampleDesc.Quality = 0;
		dxgiSwapChainDesc.Scaling = DXGI_SCALING_NONE;
		dxgiSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		dxgiSwapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
		dxgiSwapChainDesc.Flags = m_bAllowTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
	}

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc{};
	fsSwapChainDesc.Windowed = TRUE;

	ComPtr<IDXGISwapChain1> pSwapChain1;
	HRESULT hr = DXGI_FACTORY->CreateSwapChainForHwnd(
		m_pd3dCommandQueue.Get(),
		WinCore::g_hWnd,
		&dxgiSwapChainDesc,
		&fsSwapChainDesc,
		nullptr,
		pSwapChain1.GetAddressOf());

	if (FAILED(hr)) {
		SHOW_ERROR("Failed to create SwapChain");
	}

	hr = pSwapChain1->QueryInterface(IID_PPV_ARGS(m_pdxgiSwapChain.GetAddressOf()));
	if (FAILED(hr)) {
		SHOW_ERROR("Failed to create SwapChain QueryInterface");
	}

	m_unBackBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();

	hr = pFactory->MakeWindowAssociation(WinCore::g_hWnd, DXGI_MWA_NO_ALT_ENTER);
	if (FAILED(hr)) {
		SHOW_ERROR("Failed to disable DXGI Alt+Enter");
	}
}

void RenderManager::CreateRenderTarget()
{
	HRESULT hr{};
	for (UINT i = 0; i < g_unMaxPendingFrames; ++i)
	{
		ComPtr<ID3D12Resource> pd3dRenderTarget;
		hr = m_pdxgiSwapChain->GetBuffer(i, IID_PPV_ARGS(pd3dRenderTarget.GetAddressOf()));
		if (FAILED(hr)) {
			SHOW_ERROR("Failed to get buffer from SwapChain");
		}

		m_BackBufferIDs[i] = TEXTURE->LoadRenderTargetTexture(pd3dRenderTarget, DXGI_FORMAT_UNKNOWN, m_dxgiRenderTargetFormat);
	}
}

void RenderManager::CreateDepthStencil()
{
	//m_DepthStencilID = TEXTURE->LoadDepthStencilTexture(
	//	"DSV",
	//	WinCore::g_dwClientWidth,
	//	WinCore::g_dwClientHeight,
	//	DXGI_FORMAT_R24_UNORM_X8_TYPELESS,
	//	DXGI_FORMAT_D24_UNORM_S8_UINT
	//);

	m_DepthStencilID = TEXTURE->LoadDepthStencilTexture(
		"DSV",
		WinCore::g_dwClientWidth,
		WinCore::g_dwClientHeight,
		DXGI_FORMAT_R32_FLOAT,
		DXGI_FORMAT_D32_FLOAT
	);
}

void RenderManager::CreateResolutionDependentResources()
{
	m_GBuffers.Initialize();
	m_PostProcessingResources.Initialize();

	m_HDRRenderTargetIDs[0] = TEXTURE->LoadRenderTargetTexture(
		"HDR0" + std::to_string(0),
		WinCore::g_dwClientWidth,
		WinCore::g_dwClientHeight,
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		DXGI_FORMAT_R16G16B16A16_FLOAT
	);

	m_HDRRenderTargetIDs[1] = TEXTURE->LoadRenderTargetTexture(
		"HDR1" + std::to_string(0),
		WinCore::g_dwClientWidth,
		WinCore::g_dwClientHeight,
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		DXGI_FORMAT_R16G16B16A16_FLOAT
	);

	m_LDRRenderTargetIDs = TEXTURE->LoadRenderTargetTexture(
		"LDR" + std::to_string(0),
		WinCore::g_dwClientWidth,
		WinCore::g_dwClientHeight,
		m_dxgiRenderTargetFormat,
		m_dxgiRenderTargetFormat
	);
}

void RenderManager::ReleaseResolutionDependentResources()
{
	for (auto& backBuffer : m_BackBufferIDs)
		backBuffer = {};

	m_DepthStencilID = {};
	m_GBuffers.Reset();
	m_PostProcessingResources.Reset();

	for (auto& hdrBuffer : m_HDRRenderTargetIDs)
		hdrBuffer = {};

	m_LDRRenderTargetIDs = {};
}

void RenderManager::Present()
{
	// 0 : V-Sync OFF
	// 1 : V-Sync ON
	uint32 unSyncInterval = D3DCore::g_unSyncInterval;
	uint32 unPresentFlags = 0;
	if (!unSyncInterval && m_bAllowTearing) {
		// if V-Sync is OFF
		unPresentFlags |= DXGI_PRESENT_ALLOW_TEARING;
	}

	HRESULT hr{};
	hr = m_pdxgiSwapChain->Present(unSyncInterval, unPresentFlags);

	if (FAILED(hr)) {
		const HRESULT hRemovedReason = DEVICE->GetDeviceRemovedReason();
		const char* pszRemovedReason = "UNKNOWN";
		switch (hRemovedReason) {
		case DXGI_ERROR_DEVICE_HUNG:
			pszRemovedReason = "DXGI_ERROR_DEVICE_HUNG";
			break;
		case DXGI_ERROR_DEVICE_REMOVED:
			pszRemovedReason = "DXGI_ERROR_DEVICE_REMOVED";
			break;
		case DXGI_ERROR_DEVICE_RESET:
			pszRemovedReason = "DXGI_ERROR_DEVICE_RESET";
			break;
		case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
			pszRemovedReason = "DXGI_ERROR_DRIVER_INTERNAL_ERROR";
			break;
		case DXGI_ERROR_INVALID_CALL:
			pszRemovedReason = "DXGI_ERROR_INVALID_CALL";
			break;
		}

		const std::string strError = std::format(
			"Present failed. HRESULT=0x{:08X}, DeviceRemovedReason={} (0x{:08X})\n",
			static_cast<uint32>(hr),
			pszRemovedReason,
			static_cast<uint32>(hRemovedReason)
		);
		::MessageBoxA(::GetActiveWindow(), strError.c_str(), "Direct3D 12 Device Removed", MB_OK | MB_ICONERROR);
		__debugbreak();
	}

	Fence();

	m_unBackBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();

	// Prepare next frame
	uint32 unNextContextIndex = (m_unCurrentContextIndex + 1) % g_unMaxPendingFrames;
	WaitForFenceValue(m_un64LastFenceValues[unNextContextIndex]);

	m_unCurrentContextIndex = unNextContextIndex;
}

uint64 RenderManager::Fence()
{
	m_un64FenceValues++;
	m_pd3dCommandQueue->Signal(m_pd3dFence.Get(), m_un64FenceValues);
	m_un64LastFenceValues[m_unCurrentContextIndex] = m_un64FenceValues;
	return m_un64FenceValues;
}

void RenderManager::WaitForFenceValue(uint64 un64ExpectedFenceValue)
{
	if (m_pd3dFence->GetCompletedValue() < un64ExpectedFenceValue)
	{
		m_pd3dFence->SetEventOnCompletion(un64ExpectedFenceValue, m_hFenceEvent);
		WaitForSingleObject(m_hFenceEvent, INFINITE);
	}
}

void RenderManager::WaitForGPUComplete()
{
	Fence();
	for (uint32 i = 0; i < g_unMaxPendingFrames; ++i) {
		WaitForFenceValue(m_un64LastFenceValues[i]);
	}
}

void RenderManager::ImmediateStateTransition(const ComPtr<ID3D12Resource>& pResource, OUT D3D12_RESOURCE_STATES& outd3dState, D3D12_RESOURCE_STATES d3dTargetState)
{
	if (!pResource) return;
	if (outd3dState == d3dTargetState) return;

	const uint64 un64CompleteFenceValue = m_pd3dFence->GetCompletedValue();
	
	auto* pCmdPair = m_ImmediateTransitionCmdLists.AllocateSafe(un64CompleteFenceValue);
	if (!pCmdPair) {
		//__debugbreak();
		return ImmediateStateTransition(pResource, outd3dState, d3dTargetState);	// Recursive Retry
	}

	auto cmdList = pCmdPair->pd3dCommandList;

	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(pResource.Get(), outd3dState, d3dTargetState);

	cmdList->ResourceBarrier(1, &barrier);
	cmdList->Close();

	ID3D12CommandList* ppLists[] = { cmdList.Get() };
	m_pd3dCommandQueue->ExecuteCommandLists(1, ppLists);

	const uint64 un64FenceValue = Fence();
	m_ImmediateTransitionCmdLists.MarkSubmitted(pCmdPair->id, un64FenceValue);


	outd3dState = d3dTargetState;
}

void RenderManager::ChangeSwapChainState()
{
	// TODO : ResourceTable 에서 리소스 삭제 구현 후 구현
}
