#pragma once
#include "RenderPass.h"
#include "ParticleRenderBatch.h"

class ParticlePass : public IRenderPass {
private:
	struct ParticleDrawUnit {
		PARTICLE_BLEND_MODE eBlendMode = PARTICLE_BLEND_MODE::ALPHA_BLEND;

		//std::vector<const TextureRef<Texture>*> pTextureRefs;
		IndexMap<Texture::ID, const TextureRef<Texture>*> textureMap;
		std::vector<ParticleDrawData> drawDatas;

		void Clear() {
			textureMap.Clear();
			drawDatas.clear();
		}

		void Reserve(size_t textureCount, size_t particleCount) {
			textureMap.Reserve(textureCount);
			drawDatas.reserve(particleCount);
		}
	};

	struct AlphaParticle {
		const TextureRef<Texture>* textureRef;
		ParticleDrawData drawData;
		float fDistanceSq = 0.f;
	};

public:
	virtual void Initialize() override;

	virtual void OnPreRender(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		const RenderPassInput& input,
		OUT RenderPassOutput& output,
		OUT DescriptorHandle& outDescHandle) override;

	virtual void Render(
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
	void BuildDrawUnits();

	void BuildAdditiveDrawUnit(const std::vector<ParticleRenderBatch>& batches);
	void BuildAlphaDrawUnit(const std::vector<ParticleRenderBatch>& batches);

	void AppendBatchToDrawUnit(
		ParticleDrawUnit& drawUnit,
		const ParticleRenderBatch& batch);

	uint32 InsertTextureAndGetIndex(
		ParticleDrawUnit& drawUnit,
		const TextureRef<Texture>& textureRef);

	void DrawUnit(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		const ParticleDrawUnit& drawUnit,
		OUT DescriptorHandle& outDescHandle);

	void BindParticleDrawUnit(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		const ParticleDrawUnit& drawUnit,
		OUT DescriptorHandle& outDescHandle);


private:
	void CreatePipelineStates();
	void SetRenderTargets(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList) const;


private:
	ComPtr<ID3D12PipelineState> m_pd3dAdditivePipelineState;
	ComPtr<ID3D12PipelineState> m_pd3dBlendPipelineState;

private:
	ParticleDrawUnit m_AdditiveDrawUnit;
	ParticleDrawUnit m_AlphaDrawUnit;

	std::vector<AlphaParticle> m_AlphaParticles;
	uint32 m_unAdditiveParticles = 0;
	uint32 m_unAlphaParticles = 0;
};

