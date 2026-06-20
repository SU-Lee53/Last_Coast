#pragma once
#include "RenderPass.h"


// TODO : ping-pong 을 위한 HDR 버퍼가 1개 더 필요
// 이 Pass 를 지금 상태에서 만들면 1개의 HDR 버퍼를 RTV/SRV로 동시에 읽고 써야 하는 문제가 있음
// 한번 복사를 거치던가, HDR 버퍼를 하나 더 파서 ping-pong 으로 구현해야함
// 아무래도 속도 면에서는 ping-pong 이 더 좋을거같음

class DeferredFogPass : public IRenderPass {
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
	void CreatePipelineState();

private:
	ComPtr<ID3D12PipelineState> m_pd3dPipelineState;
};
