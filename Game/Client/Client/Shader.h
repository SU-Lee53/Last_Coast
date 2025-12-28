#pragma once


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Shader

class Shader {
public:
	virtual void Initialize(ComPtr<ID3D12Device> pd3dDevice, 
		ComPtr<ID3D12RootSignature> pd3dRootSignature = nullptr) = 0;

	const std::vector<ComPtr<ID3D12PipelineState>>&
		GetPipelineStates() const { return m_pd3dPipelineStates; }

	virtual void SetRootSignature(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList) {}

protected:
	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout() { return D3D12_INPUT_LAYOUT_DESC{}; }
	virtual D3D12_RASTERIZER_DESC CreateRasterizerState();
	virtual D3D12_BLEND_DESC CreateBlendState();
	virtual D3D12_DEPTH_STENCIL_DESC CreateDepthStencilState();

	D3D12_SHADER_BYTECODE CompileShaderFromFile(const std::wstring& wstrFileName, const std::string& strShaderName, const std::string& strShaderProfile, ID3DBlob** ppd3dShaderBlob);
	D3D12_SHADER_BYTECODE ReadCompiledShaderFromFile(const std::wstring& wstrFileName, ID3DBlob** ppd3dShaderBlob);

protected:
	ComPtr<ID3DBlob> m_pd3dVertexShaderBlob = nullptr;
	ComPtr<ID3DBlob> m_pd3dGeometryShaderBlob = nullptr;
	ComPtr<ID3DBlob> m_pd3dPixelShaderBlob = nullptr;

	std::vector<ComPtr<ID3D12PipelineState>>	m_pd3dPipelineStates = {};

	std::vector<D3D12_INPUT_ELEMENT_DESC>		m_d3dInputElements = {};

public:
	static D3D12_SHADER_BYTECODE CompileShader(const std::wstring& wstrFileName, const std::string& strShaderName, const std::string& strShaderProfile, ID3DBlob** ppd3dShaderBlob);

};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DiffusedShader

class DiffusedShader : public Shader {
public:
	virtual void Initialize(ComPtr<ID3D12Device> pd3dDevice, 
		ComPtr<ID3D12RootSignature> pd3dRootSignature = nullptr) override;

protected:
	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout() override;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TexturedShader
// 텍스쳐 맵핑 "만"

class TexturedShader : public Shader {
public:
	virtual void Initialize(ComPtr<ID3D12Device> pd3dDevice, 
		ComPtr<ID3D12RootSignature> pd3dRootSignature = nullptr) override;

protected:
	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout() override;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TexturedIlluminatedShader
// 텍스쳐 맵핑 + 조명 처리

class TexturedIlluminatedShader : public Shader {
public:
	virtual void Initialize(ComPtr<ID3D12Device> pd3dDevice, 
		ComPtr<ID3D12RootSignature> pd3dRootSignature = nullptr) override;

protected:
	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout() override;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TexturedNormalShader
// 텍스쳐 맵핑 + 조명 처리 + Normal Mapping

class TexturedNormalShader : public Shader {
public:
	virtual void Initialize(ComPtr<ID3D12Device> pd3dDevice, 
		ComPtr<ID3D12RootSignature> pd3dRootSignature = nullptr) override;

protected:
	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout() override;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FullScreenShader

class FullScreenShader : public Shader {
public:
	virtual void Initialize(ComPtr<ID3D12Device> pd3dDevice,
		ComPtr<ID3D12RootSignature> pd3dRootSignature = nullptr) override;

protected:
	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout() override;
	void CreateRootSignature(ComPtr<ID3D12Device> pd3dDevice);

	virtual void SetRootSignature(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList) override {
		pd3dCommandList->SetGraphicsRootSignature(m_pd3dFullScreenRootSignature.Get());
	}

protected:
	ComPtr<ID3D12RootSignature> m_pd3dFullScreenRootSignature;
};
