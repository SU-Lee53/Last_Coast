#pragma once
#include "Scene.h"
#include "StructuredBuffer.h"

struct InstancePair;
struct RenderTargetTexture;

struct CB_PER_OBJECT_DATA {
	MaterialColors materialColors;
	int nInstanceBase;
};

class RenderPass {
public:
	RenderPass() {}
	virtual ~RenderPass() {}

	virtual void Run(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, std::span<InstancePair> instances, DescriptorHandle& descHandleFromPassStart) = 0;

protected:
	std::vector<RenderTargetTexture> m_pRTVs;			// for MRT

};

class ForwardPass : public RenderPass {
public:
	ForwardPass(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommmandList);
	virtual ~ForwardPass() {}

	virtual void Run(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, std::span<InstancePair> instances, DescriptorHandle& descHandleFromPassStart) override;

protected:
	StructuredBuffer m_InstanceSBuffer;

};