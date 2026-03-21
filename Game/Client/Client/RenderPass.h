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
	std::vector<std::shared_ptr<RenderTargetTexture>> pRenderTargets;
	RenderPassResource passResource;
};

struct RenderPassOutput {
	std::vector<std::shared_ptr<RenderTargetTexture>> pRenderTargets;
	RenderPassResource passResource;

	RenderPassInput ToInput() {
		return RenderPassInput{ pRenderTargets, passResource };
	}
};

interface IRenderPass abstract {
public:
	using RenderTargetArray = std::array<std::vector<std::shared_ptr<RenderTargetTexture>>, 3>;

public:
	virtual void Initialize() {};

	void Execute(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		const RenderPassInput& input, 
		OUT RenderPassOutput& output, 
		OUT DescriptorHandle& outDescHandle);

	void Connect(std::shared_ptr<IRenderPass> pNode);
	const std::list<std::shared_ptr<IRenderPass>>& GetEdges() const { return m_pEdgeList; }

	virtual void ShowDebugInfo() const { ImGui::Text("Undefined"); }

protected:
	virtual void Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle) const = 0;
	virtual void OnPreRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle) const = 0;
	virtual void OnPostRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle) const = 0;

protected:
	//RenderTargetArray m_pRTVs;			// for MRT

	std::list<std::shared_ptr<IRenderPass>> m_pEdgeList;
};

class PostProcessingHDRPass: public IRenderPass {
public:
	virtual void Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle) const override {}
	virtual void OnPreRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle) const override {}
	virtual void OnPostRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle) const override {}

};

class PostProcessingLDRPass: public IRenderPass {
public:
	virtual void Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle) const override {}
	virtual void OnPreRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle) const override {}
	virtual void OnPostRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle) const override {}

};
