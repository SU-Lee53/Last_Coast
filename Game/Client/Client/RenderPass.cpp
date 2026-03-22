#include "pch.h"
#include "RenderPass.h"

void IRenderPass::Execute(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle)
{
	OnPreRender(pd3dCommandList, input, output, outDescHandle);
	Render(pd3dCommandList, input, output, outDescHandle);
	OnPostRender(pd3dCommandList, input, output, outDescHandle);
}

void IRenderPass::Connect(std::shared_ptr<IRenderPass> pNode)
{
	m_pEdgeList.push_back(pNode);
}
