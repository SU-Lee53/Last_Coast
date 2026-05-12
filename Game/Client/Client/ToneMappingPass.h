#pragma once
#include "RenderPass.h"

class ToneMappingPass : public IRenderPass {
private:
	enum class TONE_MAPPING_MODE : uint32 { AGX = 0, ACES, UC2, GT, COUNT, UNDEFINED = std::numeric_limits<uint32>::max() };
	enum class DIRTY_UPDATE : uint32 { AGX = 0, ACES, UC2, GT, GRADING };


public:
	virtual void Initialize() override;

	virtual void Render(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		const RenderPassInput& input,
		OUT RenderPassOutput& output,
		OUT DescriptorHandle& outDescHandle) override;

	virtual void OnPreRender(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		const RenderPassInput& input, 
		OUT RenderPassOutput& output, 
		OUT DescriptorHandle& outDescHandle) override;


	virtual void OnPostRender(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		const RenderPassInput& input, 
		OUT RenderPassOutput& output, 
		OUT DescriptorHandle& outDescHandle) override;

	virtual void ShowDebugInfo() override;

private:
	void CreatePipelineState();
	void CreateRootSignature();

private:
	ComPtr<ID3D12PipelineState> m_pd3dPipelineState;

	ComPtr<ID3D12RootSignature> m_pd3dLUTRootSignature;
	ComPtr<ID3D12PipelineState> m_pd3dLUTBakingPipelineState[2];	// 0 : Tone map LUT / 1 : Look LUT

	TextureRef<UnorderedAccessTexture> m_ToneMapLUT;
	TextureRef<UnorderedAccessTexture> m_GradingLUT;

	float m_fLastToneLUTUpdated = 0.f;
	float m_fLastLookLUTUpdated = 0.f;

	inline const static std::string g_strSavePath = "../Resources/ToneMappings";
	const static uint32 g_unLUTSize = 32;
};
