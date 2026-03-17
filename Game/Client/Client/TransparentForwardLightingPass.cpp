#include "pch.h"
#include "TransparentForwardLightingPass.h"

void TransparentForwardLightingPass::Initialize()
{
}

void TransparentForwardLightingPass::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle) const
{
}

void TransparentForwardLightingPass::OnPreRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle) const
{
}

void TransparentForwardLightingPass::OnPostRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle) const
{
}

void TransparentForwardLightingPass::CreatePipelineState()
{
}
