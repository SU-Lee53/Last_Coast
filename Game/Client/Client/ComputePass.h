#pragma once


struct ComputePassInput {
	std::vector<std::shared_ptr<Texture>> pSRVs;
	std::vector<std::shared_ptr<UnorderedAccessTexture>> pUAVs;
	void* pAdditionalContext;
};


interface IComputePass abstract {
public:
	void Initialize();

	virtual void Dispatch(
		const ComputePassInput& input,
		uint32 unNumThreadX, 
		uint32 unNumThreadY,
		uint32 unNumThreadZ) = 0;

	virtual void Dispatch(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		const ComputePassInput& input,
		uint32 unNumThreadX,
		uint32 unNumThreadY,
		uint32 unNumThreadZ) = 0;

protected:
	virtual void CreateRootSignature() = 0;
	virtual void CreatePipelineState() = 0;

protected:
	ComPtr<ID3D12RootSignature> m_pd3dRootSignature;
	ComPtr<ID3D12PipelineState> m_pd3dPipelineState;

};

class HDRIToCubeMapPass : public IComputePass {
public:
	struct CB_SKYBOX_SIZE {
		int nWidth;
		int nHeight;
	};

public:
	virtual void Dispatch(
		const ComputePassInput& input,
		uint32 unNumThreadX,
		uint32 unNumThreadY,
		uint32 unNumThreadZ) override;
	
	virtual void Dispatch(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		const ComputePassInput& input,
		uint32 unNumThreadX,
		uint32 unNumThreadY,
		uint32 unNumThreadZ) override { };

private:
	virtual void CreateRootSignature() override;
	virtual void CreatePipelineState() override;
};
