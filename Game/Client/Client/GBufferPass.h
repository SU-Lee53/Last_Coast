#pragma once
#include "RenderPass.h"

class GBufferPass : public IRenderPass {
	struct RenderParameter {
		CB_INSTANCE_DATA cbInstanceData;
		std::vector<Matrix> sbWorldTransformData;
		std::vector<AnimationController*> pAnimationControllers;
	};

	using RenderQueue = std::vector<std::pair<IMesh*, GBufferPass::RenderParameter>>;

public:
	virtual void Initialize() override;

	virtual void Render(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		const RenderPassInput& input,
		OUT RenderPassOutput& output,
		OUT DescriptorHandle& outDescHandle) const override;

	virtual void OnPreRender(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		const RenderPassInput& input,
		OUT DescriptorHandle& outDescHandle) const override;


	virtual void OnPostRender(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		const RenderPassInput& input,
		OUT DescriptorHandle& outDescHandle) const override;

public:
	void SetRenderTargets(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList) const;

private:
	void BindGeometryData(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		const std::vector<std::shared_ptr<IGameObject>>& frustumCulled, 
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

};
