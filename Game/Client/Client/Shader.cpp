#include "pch.h"
#include "Shader.h"
#include <d3dcompiler.h>

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Shader

D3D12_RASTERIZER_DESC Shader::CreateRasterizerState()
{
	return CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
}

D3D12_BLEND_DESC Shader::CreateBlendState()
{
	return CD3DX12_BLEND_DESC(D3D12_DEFAULT);
}

D3D12_DEPTH_STENCIL_DESC Shader::CreateDepthStencilState()
{
	return CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
}

D3D12_SHADER_BYTECODE Shader::CompileShaderFromFile(const std::wstring& wstrFileName, const std::string& strShaderName, const std::string& strShaderProfile, ID3DBlob** ppd3dShaderBlob)
{
	UINT nCompileFlags = 0;
#ifdef _DEBUG
	nCompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	ComPtr<ID3DBlob> pd3dErrorBlob = nullptr;
	HRESULT hResult = ::D3DCompileFromFile(wstrFileName.data(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, strShaderName.data(), strShaderProfile.data(), nCompileFlags, 0, ppd3dShaderBlob, pd3dErrorBlob.GetAddressOf());
	if (FAILED(hResult)) {
		char* pErrorString = NULL;
		if (pd3dErrorBlob) {
			pErrorString = (char*)pd3dErrorBlob->GetBufferPointer();
			HWND hWnd = ::GetActiveWindow();
			MessageBoxA(hWnd, pErrorString, NULL, 0);
			OutputDebugStringA(pErrorString);
		}
		__debugbreak();
	}
	
	char* pErrorString = NULL;
	if (pd3dErrorBlob) {
		pErrorString = (char*)pd3dErrorBlob->GetBufferPointer();
		HWND hWnd = ::GetActiveWindow();
		MessageBoxA(hWnd, pErrorString, NULL, 0);
		OutputDebugStringA(pErrorString);
		__debugbreak();
	}

	D3D12_SHADER_BYTECODE d3dShaderByteCode{};
	d3dShaderByteCode.BytecodeLength = (*ppd3dShaderBlob)->GetBufferSize();
	d3dShaderByteCode.pShaderBytecode = (*ppd3dShaderBlob)->GetBufferPointer();

	return(d3dShaderByteCode);
}

D3D12_SHADER_BYTECODE Shader::ReadCompiledShaderFromFile(const std::wstring& wstrFileName, ID3DBlob** ppd3dShaderBlob)
{
	std::ifstream in{ wstrFileName.data(), std::ios::binary };

	if (!in) {
		__debugbreak();
	}

	in.seekg(0, std::ios::end);
	int nFileSize = in.tellg();
	in.seekg(0, std::ios::beg);

	std::unique_ptr<BYTE[]> pByteCode = std::make_unique<BYTE[]>(nFileSize);
	in.read((char*)pByteCode.get(), nFileSize);

	D3D12_SHADER_BYTECODE d3dShaderByteCode{};

	HRESULT hr = D3DCreateBlob(nFileSize, ppd3dShaderBlob);
	if (FAILED(hr)) {
		__debugbreak();
	}

	::memcpy((*ppd3dShaderBlob)->GetBufferPointer(), pByteCode.get(), nFileSize);

	d3dShaderByteCode.BytecodeLength = (*ppd3dShaderBlob)->GetBufferSize();
	d3dShaderByteCode.pShaderBytecode = (*ppd3dShaderBlob)->GetBufferPointer();

	return d3dShaderByteCode;
}

D3D12_SHADER_BYTECODE Shader::CompileShader(const std::wstring& wstrFileName, const std::string& strShaderName, const std::string& strShaderProfile, ID3DBlob** ppd3dShaderBlob)
{
	UINT nCompileFlags = 0;
#ifdef _DEBUG
	nCompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	ComPtr<ID3DBlob> pd3dErrorBlob = nullptr;
	HRESULT hResult = ::D3DCompileFromFile(wstrFileName.data(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, strShaderName.data(), strShaderProfile.data(), nCompileFlags, 0, ppd3dShaderBlob, pd3dErrorBlob.GetAddressOf());
	if (FAILED(hResult)) {
		char* pErrorString = NULL;
		if (pd3dErrorBlob) {
			pErrorString = (char*)pd3dErrorBlob->GetBufferPointer();
			HWND hWnd = ::GetActiveWindow();
			MessageBoxA(hWnd, pErrorString, NULL, 0);
			OutputDebugStringA(pErrorString);
		}
		__debugbreak();
	}

	char* pErrorString = NULL;
	if (pd3dErrorBlob) {
		pErrorString = (char*)pd3dErrorBlob->GetBufferPointer();
		HWND hWnd = ::GetActiveWindow();
		MessageBoxA(hWnd, pErrorString, NULL, 0);
		OutputDebugStringA(pErrorString);
		__debugbreak();
	}

	D3D12_SHADER_BYTECODE d3dShaderByteCode{};
	d3dShaderByteCode.BytecodeLength = (*ppd3dShaderBlob)->GetBufferSize();
	d3dShaderByteCode.pShaderBytecode = (*ppd3dShaderBlob)->GetBufferPointer();

	return(d3dShaderByteCode);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DiffusedShader

void DiffusedShader::Initialize(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12RootSignature> pd3dRootSignature)
{
	m_pd3dPipelineStates.resize(1);

	// Pipeline #0 : Instancing
	D3D12_GRAPHICS_PIPELINE_STATE_DESC d3dPipelineDesc{};
	{
		d3dPipelineDesc.pRootSignature = (pd3dRootSignature) ? pd3dRootSignature.Get() : RenderManager::g_pd3dGlobalRootSignature.Get();
		d3dPipelineDesc.VS = SHADER->GetShaderByteCode("VSDiffused");
		d3dPipelineDesc.PS = SHADER->GetShaderByteCode("PSDiffused");
		d3dPipelineDesc.RasterizerState = CreateRasterizerState();
		d3dPipelineDesc.BlendState = CreateBlendState();
		d3dPipelineDesc.DepthStencilState = CreateDepthStencilState();
		d3dPipelineDesc.InputLayout = CreateInputLayout();
		d3dPipelineDesc.SampleMask = UINT_MAX;
		d3dPipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		d3dPipelineDesc.NumRenderTargets = 1;
		d3dPipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		d3dPipelineDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		d3dPipelineDesc.SampleDesc.Count = 1;
		d3dPipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	}

	HRESULT hr = pd3dDevice->CreateGraphicsPipelineState(&d3dPipelineDesc, IID_PPV_ARGS(m_pd3dPipelineStates[0].GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
	}
}

D3D12_INPUT_LAYOUT_DESC DiffusedShader::CreateInputLayout()
{
	m_d3dInputElements = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc;
	inputLayoutDesc.NumElements = m_d3dInputElements.size();
	inputLayoutDesc.pInputElementDescs = m_d3dInputElements.data();

	return inputLayoutDesc;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TexturedShader

void TexturedShader::Initialize(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12RootSignature> pd3dRootSignature)
{
	m_pd3dPipelineStates.resize(1);

	// Pipeline #0 : Instancing
	D3D12_GRAPHICS_PIPELINE_STATE_DESC d3dPipelineDesc{};
	{
		d3dPipelineDesc.pRootSignature = (pd3dRootSignature) ? pd3dRootSignature.Get() : RenderManager::g_pd3dGlobalRootSignature.Get();
		d3dPipelineDesc.VS = SHADER->GetShaderByteCode("VSTextured");
		d3dPipelineDesc.PS = SHADER->GetShaderByteCode("PSTextured");
		d3dPipelineDesc.RasterizerState = CreateRasterizerState();
		d3dPipelineDesc.BlendState = CreateBlendState();
		d3dPipelineDesc.DepthStencilState = CreateDepthStencilState();
		d3dPipelineDesc.InputLayout = CreateInputLayout();
		d3dPipelineDesc.SampleMask = UINT_MAX;
		d3dPipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		d3dPipelineDesc.NumRenderTargets = 1;
		d3dPipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		d3dPipelineDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		d3dPipelineDesc.SampleDesc.Count = 1;
		d3dPipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	}

	HRESULT hr = pd3dDevice->CreateGraphicsPipelineState(&d3dPipelineDesc, IID_PPV_ARGS(m_pd3dPipelineStates[0].GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
	}
}

D3D12_INPUT_LAYOUT_DESC TexturedShader::CreateInputLayout()
{
	m_d3dInputElements = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc;
	inputLayoutDesc.NumElements = m_d3dInputElements.size();
	inputLayoutDesc.pInputElementDescs = m_d3dInputElements.data();

	return inputLayoutDesc;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TexturedNormalShader

void TexturedIlluminatedShader::Initialize(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12RootSignature> pd3dRootSignature)
{
	m_pd3dPipelineStates.resize(1);

	// Pipeline #0 : Instancing
	D3D12_GRAPHICS_PIPELINE_STATE_DESC d3dPipelineDesc{};
	{
		d3dPipelineDesc.pRootSignature = (pd3dRootSignature) ? pd3dRootSignature.Get() : RenderManager::g_pd3dGlobalRootSignature.Get();
		d3dPipelineDesc.VS = SHADER->GetShaderByteCode("TexturedIlluminatedVS");
		d3dPipelineDesc.PS = SHADER->GetShaderByteCode("TexturedIlluminatedPS");
		d3dPipelineDesc.RasterizerState = CreateRasterizerState();
		d3dPipelineDesc.BlendState = CreateBlendState();
		d3dPipelineDesc.DepthStencilState = CreateDepthStencilState();
		d3dPipelineDesc.InputLayout = CreateInputLayout();
		d3dPipelineDesc.SampleMask = UINT_MAX;
		d3dPipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		d3dPipelineDesc.NumRenderTargets = 1;
		d3dPipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		d3dPipelineDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		d3dPipelineDesc.SampleDesc.Count = 1;
		d3dPipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	}

	HRESULT hr = pd3dDevice->CreateGraphicsPipelineState(&d3dPipelineDesc, IID_PPV_ARGS(m_pd3dPipelineStates[0].GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
	}
}

D3D12_INPUT_LAYOUT_DESC TexturedIlluminatedShader::CreateInputLayout()
{
	m_d3dInputElements = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc;
	inputLayoutDesc.NumElements = m_d3dInputElements.size();
	inputLayoutDesc.pInputElementDescs = m_d3dInputElements.data();

	return inputLayoutDesc;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TexturedNormalShader

void TexturedNormalShader::Initialize(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12RootSignature> pd3dRootSignature)
{
	m_pd3dPipelineStates.resize(1);

	// Pipeline #0 : Instancing
	D3D12_GRAPHICS_PIPELINE_STATE_DESC d3dPipelineDesc{};
	{
		d3dPipelineDesc.pRootSignature = (pd3dRootSignature) ? pd3dRootSignature.Get() : RenderManager::g_pd3dGlobalRootSignature.Get();
		d3dPipelineDesc.VS = SHADER->GetShaderByteCode("TexturedNormalVS");
		d3dPipelineDesc.PS = SHADER->GetShaderByteCode("TexturedNormalGS");
		d3dPipelineDesc.RasterizerState = CreateRasterizerState();
		d3dPipelineDesc.BlendState = CreateBlendState();
		d3dPipelineDesc.DepthStencilState = CreateDepthStencilState();
		d3dPipelineDesc.InputLayout = CreateInputLayout();
		d3dPipelineDesc.SampleMask = UINT_MAX;
		d3dPipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		d3dPipelineDesc.NumRenderTargets = 1;
		d3dPipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		d3dPipelineDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		d3dPipelineDesc.SampleDesc.Count = 1;
		d3dPipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	}

	HRESULT hr = pd3dDevice->CreateGraphicsPipelineState(&d3dPipelineDesc, IID_PPV_ARGS(m_pd3dPipelineStates[0].GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
	}
}

D3D12_INPUT_LAYOUT_DESC TexturedNormalShader::CreateInputLayout()
{
	m_d3dInputElements = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc;
	inputLayoutDesc.NumElements = m_d3dInputElements.size();
	inputLayoutDesc.pInputElementDescs = m_d3dInputElements.data();

	return inputLayoutDesc;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FullScreenShader

void FullScreenShader::Initialize(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12RootSignature> pd3dRootSignature)
{
	m_pd3dPipelineStates.resize(1);

	CreateRootSignature(pd3dDevice);

	// Pipeline #0 : No Instancing
	D3D12_GRAPHICS_PIPELINE_STATE_DESC d3dPipelineDesc{};
	{
		d3dPipelineDesc.pRootSignature = (pd3dRootSignature) ? pd3dRootSignature.Get() : RenderManager::g_pd3dGlobalRootSignature.Get();
		d3dPipelineDesc.VS = SHADER->GetShaderByteCode("FullScreenVS");
		d3dPipelineDesc.PS = SHADER->GetShaderByteCode("FullScreenPS");
		d3dPipelineDesc.RasterizerState = CreateRasterizerState();
		d3dPipelineDesc.BlendState = CreateBlendState();
		d3dPipelineDesc.DepthStencilState = CreateDepthStencilState();
		d3dPipelineDesc.InputLayout = CreateInputLayout();
		d3dPipelineDesc.SampleMask = UINT_MAX;
		d3dPipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		d3dPipelineDesc.NumRenderTargets = 1;
		d3dPipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		d3dPipelineDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		d3dPipelineDesc.SampleDesc.Count = 1;
		d3dPipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	}

	HRESULT hr = pd3dDevice->CreateGraphicsPipelineState(&d3dPipelineDesc, IID_PPV_ARGS(m_pd3dPipelineStates[0].GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
	}

}

D3D12_INPUT_LAYOUT_DESC FullScreenShader::CreateInputLayout()
{
	m_d3dInputElements = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc;
	inputLayoutDesc.NumElements = m_d3dInputElements.size();
	inputLayoutDesc.pInputElementDescs = m_d3dInputElements.data();

	return inputLayoutDesc;
}

void FullScreenShader::CreateRootSignature(ComPtr<ID3D12Device> pd3dDevice)
{
	CD3DX12_DESCRIPTOR_RANGE descRange;
	descRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, 0);

	CD3DX12_ROOT_PARAMETER d3dRootParameters[2];
	d3dRootParameters[0].InitAsConstants(2, 0, 0, D3D12_SHADER_VISIBILITY_ALL);
	d3dRootParameters[1].InitAsDescriptorTable(1, &descRange, D3D12_SHADER_VISIBILITY_ALL);

	CD3DX12_STATIC_SAMPLER_DESC d3dSamplerDesc;
	d3dSamplerDesc.Init(0);
	d3dSamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	d3dSamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	d3dSamplerDesc.MinLOD = 0;
	d3dSamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	d3dSamplerDesc.ShaderRegister = 0;
	d3dSamplerDesc.RegisterSpace = 0;
	d3dSamplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_FLAGS d3dRootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	D3D12_ROOT_SIGNATURE_DESC d3dRootSignatureDesc{};
	{
		d3dRootSignatureDesc.NumParameters = _countof(d3dRootParameters);
		d3dRootSignatureDesc.pParameters = d3dRootParameters;
		d3dRootSignatureDesc.NumStaticSamplers = 1;
		d3dRootSignatureDesc.pStaticSamplers = &d3dSamplerDesc;
		d3dRootSignatureDesc.Flags = d3dRootSignatureFlags;
	}

	ComPtr<ID3DBlob> pd3dSignatureBlob = nullptr;
	ComPtr<ID3DBlob> pd3dErrorBlob = nullptr;

	HRESULT hr = D3D12SerializeRootSignature(&d3dRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, pd3dSignatureBlob.GetAddressOf(), pd3dErrorBlob.GetAddressOf());
	if (pd3dErrorBlob) {
		char* pErrorString = (char*)pd3dErrorBlob->GetBufferPointer();
		HWND hWnd = ::GetActiveWindow();
		MessageBoxA(hWnd, pErrorString, NULL, 0);
		OutputDebugStringA(pErrorString);
		__debugbreak();
	}

	pd3dDevice->CreateRootSignature(0, pd3dSignatureBlob->GetBufferPointer(), pd3dSignatureBlob->GetBufferSize(), IID_PPV_ARGS(m_pd3dFullScreenRootSignature.GetAddressOf()));

}
