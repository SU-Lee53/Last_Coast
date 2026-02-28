#pragma once
#include "Scene.h"

struct RenderPassResource {
	void* pResource;

	template<typename T>
	const T* Get() const {
		return static_cast<T*>(pResource);
	}
};

struct RenderPassInput {
	std::vector<RenderTargetTexture> pRenderTargets;
	RenderPassResource passResource;
};

struct RenderPassOutput {
	std::vector<RenderTargetTexture> pRenderTargets;
	RenderPassResource passResource;

	RenderPassInput ToInput() {
		return RenderPassInput{ pRenderTargets, passResource };
	}
};

interface IRenderPass abstract {
public:
	void Initialize();

	void Execute(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		const RenderPassInput& input, 
		OUT RenderPassOutput& output, 
		OUT DescriptorHandle& outDescHandle);

	void Connect(std::shared_ptr<IRenderPass> pNode);
	const std::list<std::shared_ptr<IRenderPass>>& GetEdges() const { return m_pEdgeList; }

protected:
	virtual void Run(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle) const = 0;
	virtual void OnPreRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT DescriptorHandle& outDescHandle) const = 0;
	virtual void OnPostRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT DescriptorHandle& outDescHandle) const = 0;


protected:
	std::vector<RenderTargetTexture> m_pRTVs;			// for MRT

	std::list<std::shared_ptr<IRenderPass>> m_pEdgeList;
};

class ForwardPass : public IRenderPass {
	struct RenderParameter {
		CB_INSTANCE_DATA cbInstanceData;
		std::vector<Matrix> sbWorldTransformData;
		std::vector<AnimationController*> pAnimationControllers;
	};

	using RenderQueue = std::vector<std::pair<IMesh*, ForwardPass::RenderParameter>>;

public:
	virtual void Run(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle) const override;
	virtual void OnPreRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT DescriptorHandle& outDescHandle) const override;
	virtual void OnPostRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT DescriptorHandle& outDescHandle) const override;

private:
	mutable RenderQueue m_RenderQueueCached;

};

class ShadowMapPass : public IRenderPass {
public:
	virtual void Run(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle) const override {}
	virtual void OnPreRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT DescriptorHandle& outDescHandle) const override {}
	virtual void OnPostRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT DescriptorHandle& outDescHandle) const override {}

};

class GBufferPass : public IRenderPass {
public:
	virtual void Run(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle) const override {}
	virtual void OnPreRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT DescriptorHandle& outDescHandle) const override {}
	virtual void OnPostRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT DescriptorHandle& outDescHandle) const override {}

};

class DefferedLightingPass: public IRenderPass {
public:
	virtual void Run(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle) const override {}
	virtual void OnPreRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT DescriptorHandle& outDescHandle) const override {}
	virtual void OnPostRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT DescriptorHandle& outDescHandle) const override {}

};

class TransparentPass: public IRenderPass {	// Forward
public:
	virtual void Run(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle) const override {}
	virtual void OnPreRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT DescriptorHandle& outDescHandle) const override {}
	virtual void OnPostRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT DescriptorHandle& outDescHandle) const override {}

};

class TerrainPass : public IRenderPass {
public:
	virtual void Run(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle) const override;
	virtual void OnPreRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT DescriptorHandle& outDescHandle) const override;
	virtual void OnPostRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT DescriptorHandle& outDescHandle) const override;

};

class SkyboxPass : public IRenderPass {
public:
	virtual void Run(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle) const override {}
	virtual void OnPreRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT DescriptorHandle& outDescHandle) const override {}
	virtual void OnPostRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT DescriptorHandle& outDescHandle) const override {}

};

class PostProcessingHDRPass: public IRenderPass {
public:
	virtual void Run(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle) const override {}
	virtual void OnPreRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT DescriptorHandle& outDescHandle) const override {}
	virtual void OnPostRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT DescriptorHandle& outDescHandle) const override {}

};

class PostProcessingLDRPass: public IRenderPass {
public:
	virtual void Run(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle) const override {}
	virtual void OnPreRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT DescriptorHandle& outDescHandle) const override {}
	virtual void OnPostRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT DescriptorHandle& outDescHandle) const override {}

};

class ToneMappingPass : public IRenderPass {
public:
	virtual void Run(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle) const override {}
	virtual void OnPreRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT DescriptorHandle& outDescHandle) const override {}
	virtual void OnPostRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT DescriptorHandle& outDescHandle) const override {}

};
