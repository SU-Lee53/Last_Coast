#pragma once

#define MAX_EFFECT_PER_DRAW 100

struct EffectParameter {
	Vector3		xmf3Position;
	float		fElapsedTime = 0.f;
	Vector3		xmf3Force;
	float		fAdditionalData = 0.f;
};

struct CB_PARTICLE_DATA {
	EffectParameter parameters[MAX_EFFECT_PER_DRAW];
};

struct ParticleVertexType {
	XMFLOAT3 xmf3Position;
	XMFLOAT4 xmf4Color;
	XMFLOAT3 xmf3InitialVelocity;
	XMFLOAT2 xmf2InitialSize;
	float fRandomValue;
	float fStartTime;
	float fLifeTime;
	float fMass;
};

class Effect : public std::enable_shared_from_this<Effect> {
public:
	Effect();

	virtual void Create(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, ComPtr<ID3D12RootSignature> pd3dRootSignature, int nParticles) {}
	void Update(float fElapsedTime);
	void Render(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, UINT nDataOffsetBaseInCBuffer, UINT nInstanceCount);

	bool IsEnd(float fTimeElapsed) const;

protected:
	//void CreateShaderVariable(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList);


protected:
	D3D12_INPUT_LAYOUT_DESC CreateInputLayout();
	virtual D3D12_RASTERIZER_DESC CreateRasterizerState();
	virtual D3D12_BLEND_DESC CreateBlendState();
	virtual D3D12_DEPTH_STENCIL_DESC CreateDepthStencilState();

	void CreatePipelineState(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12RootSignature> pd3dRootSignature = nullptr) {}

	virtual D3D12_SHADER_BYTECODE CreateVertexShader();
	virtual D3D12_SHADER_BYTECODE CreateGeometryShader();
	virtual D3D12_SHADER_BYTECODE CreatePixelShader();

protected:
	ComPtr<ID3D12Resource> m_pd3dParticleBuffer;
	ComPtr<ID3D12Resource> m_pd3dParticleUploadBuffer;

	UINT										m_nParticles = 0;
	D3D12_VERTEX_BUFFER_VIEW					m_d3dVertexBufferView;

	ComPtr<ID3D12PipelineState>					m_pd3dPipelineState;
	std::vector<D3D12_INPUT_ELEMENT_DESC>		m_d3dInputElements;

	float m_fTotalLifetime = 0.f;

	bool m_bLoop = false;

protected:
	ComPtr<ID3DBlob> m_pd3dVertexShaderBlob = nullptr;
	ComPtr<ID3DBlob> m_pd3dGeometryShaderBlob = nullptr;
	ComPtr<ID3DBlob> m_pd3dPixelShaderBlob = nullptr;

};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExplosionEffect

class ExplosionEffect : public Effect {
public:
	ExplosionEffect();
	virtual void Create(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, ComPtr<ID3D12RootSignature> pd3dRootSignature, int nParticles) override;

protected:
	void CreatePipelineState(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12RootSignature> pd3dRootSignature = nullptr);

};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RayEffect

class RayEffect : public Effect {
public:
	RayEffect();
	virtual void Create(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, ComPtr<ID3D12RootSignature> pd3dRootSignature, int nParticles) override;

protected:
	void CreatePipelineState(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12RootSignature> pd3dRootSignature = nullptr);
};
