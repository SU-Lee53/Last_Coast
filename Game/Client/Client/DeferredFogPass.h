#pragma once
#include "RenderPass.h"


// TODO : ping-pong 을 위한 HDR 버퍼가 1개 더 필요
// 이 Pass 를 지금 상태에서 만들면 1개의 HDR 버퍼를 RTV/SRV로 동시에 읽고 써야 하는 문제가 있음
// 한번 복사를 거치던가, HDR 버퍼를 하나 더 파서 ping-pong 으로 구현해야함
// 아무래도 속도 면에서는 ping-pong 이 더 좋을거같음

struct CB_FOG_DATA 
{
	Vector4 gfogColor;	// 안개 색상

	float gfFogStartDistance;	// Fog 시작 거리
	float gfFogCutOffDistance;	// 누적 계산의 최대 거리 상한선
	float gfFogDistanceDensity;	// Distance Fog 의 세기
	float gFogDistancePower;	// 거리에 따라 얼마나 빠르게 증가하는지 곡선의 형태를 조절

	float gfFogHeightDensity;			// 지면 근처 안개층의 세기
	float gfFogHeightFalloff;			// 높이에 따라 얼마나 빠르게 감소하는지 정함
	float gfFogBaseHeight;				// Height Fog 의 기준 높이
	float gfFogHeightStartDistance;		// Height Fog 의 시작 거리

	float gfFogMaxOpacity;		// fog 가 화면을 덮는 정도의 상한
	Vector3 pad;
};

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

	void SaveParametersToJson() const;
	void LoadParametersFromJson();

private:
	ComPtr<ID3D12PipelineState> m_pd3dPipelineState;
	CB_FOG_DATA m_cbFogData;

	std::string m_strSaveName;

	constexpr static CB_FOG_DATA g_DefaultParameters{
		.gfogColor = Vector4(0.62f, 0.68f, 0.72f, 1.0f),
		.gfFogStartDistance = 5.0f,
		.gfFogCutOffDistance = 0.0f,
		.gfFogDistanceDensity = 0.0035f,
		.gFogDistancePower = 1.0f,
		.gfFogHeightDensity = 0.02f,
		.gfFogHeightFalloff = 0.06f,
		.gfFogBaseHeight = 0.0f,
		.gfFogHeightStartDistance = 0.0f,
		.gfFogMaxOpacity = 0.f,
		.pad = Vector3(0.f),
	};

	inline const static std::string g_strSavePath = "../Resources/Scenes";
};
