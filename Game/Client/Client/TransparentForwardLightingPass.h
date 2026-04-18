#pragma once
#include "RenderPass.h"

class TransparentForwardLightingPass : public IRenderPass {
	struct RenderParameter {
		CB_INSTANCE_DATA cbInstanceData{};
		int32 nInstances = 0;
		int32 nBoneOffset = -1;
	};

	using RenderQueue = std::vector<std::pair<std::shared_ptr<IMesh>, TransparentForwardLightingPass::RenderParameter>>;

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

private:
	void BindGeometryData(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		const std::vector<std::shared_ptr<IGameObject>>& frustumCulled,
		OUT DescriptorHandle& outDescHandle) const;

	void CreatePipelineState();

private:
	mutable RenderQueue m_RenderQueueCached;

	ComPtr<ID3D12PipelineState> m_pd3dStandardPipelineState;
	ComPtr<ID3D12PipelineState> m_pd3dAnimatedPipelineState;

	mutable struct CachedData {
		IndexMap<MeshRenderer::ID, std::pair<const MeshRenderer*, std::vector<const IGameObject*>>> frustumCulledMap;
		IndexMap<IMaterial::ID, MaterialData> materialMap;
		IndexMap<Texture::ID, const TextureRef<Texture>*> textureMap;

		std::vector<WorldTransformData> sbWorldTransformDatas;
		std::vector<Matrix> sbBoneTransformDatas;

		struct AnimationOffset {
			int32 unOffset = 0;
		};

		std::unordered_map<const AnimationController*, AnimationOffset> animationOffsetData;

		void Clear() {
			frustumCulledMap.Clear();
			materialMap.Clear();
			textureMap.Clear();

			sbWorldTransformDatas.clear();
			sbBoneTransformDatas.clear();

			animationOffsetData.clear();
		}

	} m_CachedData;
};

