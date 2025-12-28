#include "pch.h"
#include "Effect.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Effect

Effect::Effect()
{
}

void Effect::Update(float fElapsedTime)
{
	//m_fElapsedtime += fElapsedTime;
}

void Effect::Render(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, UINT nDataOffsetBaseInCBuffer, UINT nInstanceCount)
{
	pd3dCommandList->SetGraphicsRoot32BitConstant(2, nDataOffsetBaseInCBuffer, 0);

	pd3dCommandList->SetPipelineState(m_pd3dPipelineState.Get());
	pd3dCommandList->IASetVertexBuffers(0, 1, &m_d3dVertexBufferView);
	pd3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
	pd3dCommandList->DrawInstanced(m_nParticles, nInstanceCount, 0, 0);
}

bool Effect::IsEnd(float fTimeElapsed) const
{
	return m_bLoop ? false : fTimeElapsed >= m_fTotalLifetime;
}

D3D12_INPUT_LAYOUT_DESC Effect::CreateInputLayout()
{
	//	typedef struct D3D12_INPUT_ELEMENT_DESC
	//	{
	//		LPCSTR SemanticName;
	//		UINT SemanticIndex;
	//		DXGI_FORMAT Format;
	//		UINT InputSlot;
	//		UINT AlignedByteOffset;
	//		D3D12_INPUT_CLASSIFICATION InputSlotClass;
	//		UINT InstanceDataStepRate;
	//	} 	D3D12_INPUT_ELEMENT_DESC

	m_d3dInputElements = {
		{"POSITION",	0, DXGI_FORMAT_R32G32B32_FLOAT,		0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"COLOR",		0, DXGI_FORMAT_R32G32B32A32_FLOAT,	0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},	// 0 + 12 = 12
		{"VELOCITY",	0, DXGI_FORMAT_R32G32B32_FLOAT,		0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},	// 12 + 16 = 28
		{"SIZE",		0, DXGI_FORMAT_R32G32_FLOAT,		0, 40, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},	// 28 + 12 = 40
		{"RANDOM",		0, DXGI_FORMAT_R32_FLOAT,			0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},	// 40 + 8 = 48
		{"STARTTIME",	0, DXGI_FORMAT_R32_FLOAT,			0, 52, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},	// 48 + 4 = 52
		{"LIFETIME",	0, DXGI_FORMAT_R32_FLOAT,			0, 56, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},	// 52 + 4 = 46
		{"MASS",		0, DXGI_FORMAT_R32_FLOAT,			0, 60, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},	// 56 + 4 = 60
	};

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc;
	inputLayoutDesc.NumElements = m_d3dInputElements.size();
	inputLayoutDesc.pInputElementDescs = m_d3dInputElements.data();

	return inputLayoutDesc;
}

D3D12_RASTERIZER_DESC Effect::CreateRasterizerState()
{
	D3D12_RASTERIZER_DESC desc{};
	{
		desc.FillMode = D3D12_FILL_MODE_SOLID;
		desc.CullMode = D3D12_CULL_MODE_BACK;
		desc.FrontCounterClockwise = FALSE;
		desc.DepthBias = 0;
		desc.DepthBiasClamp = 0.0f;
		desc.SlopeScaledDepthBias = 0.0f;
		desc.DepthClipEnable = TRUE;
		desc.MultisampleEnable = FALSE;
		desc.AntialiasedLineEnable = FALSE;
		desc.ForcedSampleCount = 0;
		desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	}

	return desc;
}

D3D12_BLEND_DESC Effect::CreateBlendState()
{
	D3D12_BLEND_DESC desc{};
	{
		desc.AlphaToCoverageEnable = FALSE;
		desc.IndependentBlendEnable = FALSE;
		desc.RenderTarget[0].BlendEnable = FALSE;
		desc.RenderTarget[0].LogicOpEnable = FALSE;
		desc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
		desc.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
		desc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		desc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		desc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
		desc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		desc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
		desc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	}

	return desc;
}

D3D12_DEPTH_STENCIL_DESC Effect::CreateDepthStencilState()
{
	D3D12_DEPTH_STENCIL_DESC desc{};
	{
		desc.DepthEnable = TRUE;
		desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		desc.StencilEnable = FALSE;
		desc.StencilReadMask = 0x00;
		desc.StencilWriteMask = 0x00;
		desc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		desc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		desc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		desc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;
		desc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		desc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		desc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		desc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;
	}

	return desc;
}

D3D12_SHADER_BYTECODE Effect::CreateVertexShader()
{
	return D3D12_SHADER_BYTECODE();
}

D3D12_SHADER_BYTECODE Effect::CreateGeometryShader()
{
	return D3D12_SHADER_BYTECODE();
}

D3D12_SHADER_BYTECODE Effect::CreatePixelShader()
{
	return D3D12_SHADER_BYTECODE();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExplosionEffect

ExplosionEffect::ExplosionEffect()
{
}

void ExplosionEffect::Create(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, ComPtr<ID3D12RootSignature> pd3dRootSignature, int nParticles)
{
	m_nParticles = nParticles;
	std::vector<ParticleVertexType> vertices(nParticles);
	for (int i = 0; i < vertices.size(); ++i) {
		vertices[i].xmf3Position = XMFLOAT3(0.f, 0.f, 0.f);
		vertices[i].xmf4Color.x = RandomGenerator::GenerateRandomFloatInRange(0.8f, 1.f);
		vertices[i].xmf4Color.y = RandomGenerator::GenerateRandomFloatInRange(0.1f, 0.5f);
		vertices[i].xmf4Color.z = 0.f;
		vertices[i].xmf4Color.w = 1.f;
		vertices[i].xmf3InitialVelocity.x = RandomGenerator::GenerateRandomFloatInRange(-100.f, 100.f);
		vertices[i].xmf3InitialVelocity.y = RandomGenerator::GenerateRandomFloatInRange(-100.f, 100.f);
		vertices[i].xmf3InitialVelocity.z = RandomGenerator::GenerateRandomFloatInRange(-100.f, 100.f);

		vertices[i].xmf2InitialSize.x = vertices[i].xmf2InitialSize.y = RandomGenerator::GenerateRandomFloatInRange(1.f, 2.f);
		vertices[i].fRandomValue = RandomGenerator::GenerateRandomFloatInRange(0.f, 1.f);
		vertices[i].fStartTime = 0.f;
		vertices[i].fLifeTime = RandomGenerator::GenerateRandomFloatInRange(0.5f, 5.f);
		vertices[i].fMass = vertices[i].xmf2InitialSize.x * 10;

		m_fTotalLifetime = std::max(m_fTotalLifetime, vertices[i].fLifeTime);
	}

	m_pd3dParticleBuffer = RESOURCE->CreateBufferResource(vertices.data(), vertices.size() * sizeof(ParticleVertexType),
		D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	m_d3dVertexBufferView.BufferLocation = m_pd3dParticleBuffer->GetGPUVirtualAddress();
	m_d3dVertexBufferView.StrideInBytes = sizeof(ParticleVertexType);
	m_d3dVertexBufferView.SizeInBytes = sizeof(ParticleVertexType) * nParticles;

	CreatePipelineState(pd3dDevice, pd3dRootSignature);
}

void ExplosionEffect::CreatePipelineState(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12RootSignature> pd3dRootSignature)
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC d3dPipelineDesc{};
	{
		d3dPipelineDesc.pRootSignature = pd3dRootSignature.Get();
		d3dPipelineDesc.VS = SHADER->GetShaderByteCode("ExplosionVS");
		d3dPipelineDesc.GS = SHADER->GetShaderByteCode("ExplosionGS");
		d3dPipelineDesc.PS = SHADER->GetShaderByteCode("ExplosionPS");
		d3dPipelineDesc.RasterizerState = CreateRasterizerState();
		d3dPipelineDesc.BlendState.AlphaToCoverageEnable = false;
		d3dPipelineDesc.BlendState.IndependentBlendEnable = false;
		d3dPipelineDesc.BlendState.RenderTarget[0].BlendEnable = true;
		d3dPipelineDesc.BlendState.RenderTarget[0].LogicOpEnable = false;
		d3dPipelineDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;	// 알파 블렌딩 켬
		d3dPipelineDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		d3dPipelineDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		d3dPipelineDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		d3dPipelineDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
		d3dPipelineDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		d3dPipelineDesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
		d3dPipelineDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		d3dPipelineDesc.DepthStencilState = CreateDepthStencilState();
		d3dPipelineDesc.InputLayout = CreateInputLayout();
		d3dPipelineDesc.SampleMask = UINT_MAX;
		d3dPipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
		d3dPipelineDesc.NumRenderTargets = 1;
		d3dPipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		d3dPipelineDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		d3dPipelineDesc.SampleDesc.Count = 1;
		d3dPipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	}

	HRESULT hr = pd3dDevice->CreateGraphicsPipelineState(&d3dPipelineDesc, IID_PPV_ARGS(m_pd3dPipelineState.GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RayEffect

RayEffect::RayEffect()
{
}

void RayEffect::Create(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, ComPtr<ID3D12RootSignature> pd3dRootSignature, int nParticles)
{
	m_nParticles = 1;
	m_fTotalLifetime = 0.4f;

	std::vector<ParticleVertexType> vertices(nParticles);
	// Ray 본체가 될 파티클 1개 -> xmf3Force 가 Ray 의 방향이 됨
	vertices[0].xmf3Position = XMFLOAT3(0.f, 0.f, 0.f);
	vertices[0].xmf4Color = XMFLOAT4(1.f, 0.f, 0.f, 1.f);
	vertices[0].xmf3InitialVelocity = XMFLOAT3(0.f, 0.f, 0.f);
	vertices[0].xmf2InitialSize = XMFLOAT2(0.f, 0.f);
	vertices[0].fRandomValue = 0.f;
	vertices[0].fStartTime = 0.f;
	vertices[0].fLifeTime = m_fTotalLifetime;
	vertices[0].fMass = 0.f;


	m_pd3dParticleBuffer = RESOURCE->CreateBufferResource(vertices.data(), vertices.size() * sizeof(ParticleVertexType),
		D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	m_d3dVertexBufferView.BufferLocation = m_pd3dParticleBuffer->GetGPUVirtualAddress();
	m_d3dVertexBufferView.StrideInBytes = sizeof(ParticleVertexType);
	m_d3dVertexBufferView.SizeInBytes = sizeof(ParticleVertexType) * nParticles;

	CreatePipelineState(pd3dDevice, pd3dRootSignature);
}

void RayEffect::CreatePipelineState(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12RootSignature> pd3dRootSignature)
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC d3dPipelineDesc{};
	{
		d3dPipelineDesc.pRootSignature = pd3dRootSignature.Get();
		d3dPipelineDesc.VS = SHADER->GetShaderByteCode("RayVS");
		d3dPipelineDesc.GS = SHADER->GetShaderByteCode("RayGS");
		d3dPipelineDesc.PS = SHADER->GetShaderByteCode("RayPS");
		d3dPipelineDesc.RasterizerState = CreateRasterizerState();
		d3dPipelineDesc.BlendState.AlphaToCoverageEnable = false;
		d3dPipelineDesc.BlendState.IndependentBlendEnable = false;
		d3dPipelineDesc.BlendState.RenderTarget[0].BlendEnable = true;
		d3dPipelineDesc.BlendState.RenderTarget[0].LogicOpEnable = false;
		d3dPipelineDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;	// 알파 블렌딩 켬
		d3dPipelineDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		d3dPipelineDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		d3dPipelineDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		d3dPipelineDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
		d3dPipelineDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		d3dPipelineDesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
		d3dPipelineDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		d3dPipelineDesc.DepthStencilState = CreateDepthStencilState();
		d3dPipelineDesc.InputLayout = CreateInputLayout();
		d3dPipelineDesc.SampleMask = UINT_MAX;
		d3dPipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
		d3dPipelineDesc.NumRenderTargets = 1;
		d3dPipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		d3dPipelineDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		d3dPipelineDesc.SampleDesc.Count = 1;
		d3dPipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	}

	HRESULT hr = pd3dDevice->CreateGraphicsPipelineState(&d3dPipelineDesc, IID_PPV_ARGS(m_pd3dPipelineState.GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
	}
}
