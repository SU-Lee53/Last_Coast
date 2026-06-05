#pragma once
#include "RenderPass.h"

class GBufferPass : public IRenderPass {
	struct RenderParameter {
		CB_INSTANCE_DATA cbInstanceData{};
		int32 nInstances = 0;
		uint32 unBoneOffsetStart = 0;
		uint32 unBoneOffsetCount = 0;
		ComPtr<ID3D12PipelineState> pd3dPipelineState = nullptr;
	};

	using RenderQueue = std::vector<std::pair<IMesh*, GBufferPass::RenderParameter>>;

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
	void SetRenderTargets(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList) const;

	void BindGeometryData(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		OUT DescriptorHandle& outDescHandle) const;
	
	void BindTerrainData(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		OUT DescriptorHandle& outDescHandle) const;

	void DrawGeometry(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, 
		OUT DescriptorHandle& outDescHandle) const;
	
	void DrawTerrain(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, 
		OUT DescriptorHandle& outDescHandle) const;

private:
	mutable RenderQueue m_RenderQueueCached;
	mutable uint32 m_unFrustumCulled = 0;

	mutable struct CachedData {
		std::vector<std::shared_ptr<IGameObject>> pObjFrustumCulled;
		std::vector<TerrainComponent*> pTerrainComponentFrustumCulled;

		IndexMap<MeshRenderer::ID, std::pair<const MeshRenderer*, std::vector<const IGameObject*>>> frustumCulledMap;
		IndexMap<IMaterial::ID, MaterialData> materialMap;
		IndexMap<Texture::ID, const TextureRef<Texture>*> textureMap;

		std::vector<WorldTransformData> sbWorldTransformDatas;
		std::vector<Matrix> sbBoneTransformDatas;
		std::vector<int32> nBoneOffsets;
		D3D12_GPU_VIRTUAL_ADDRESS d3dBoneOffsetGPUAddress = 0;

		struct AnimationInstancingData {
			int32 unOffset = 0;
		};

		std::unordered_map<const AnimationController*, AnimationInstancingData> animationInstancingData;

		void Clear() {
			frustumCulledMap.Clear();
			materialMap.Clear();
			textureMap.Clear();

			sbWorldTransformDatas.clear();
			sbBoneTransformDatas.clear();
			nBoneOffsets.clear();
			d3dBoneOffsetGPUAddress = 0;

			animationInstancingData.clear();

			pObjFrustumCulled.clear();
			pTerrainComponentFrustumCulled.clear();
		}

	} m_CachedData;


};
