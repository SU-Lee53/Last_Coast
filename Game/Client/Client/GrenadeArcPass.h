#pragma once
#include "RenderPass.h"

// 수류탄 예측 궤도 패스 (노란색, 배그식 점선 리본 + 착탄 마커).
// GameScene::UpdateGrenadeArcPreview()가 매 프레임 SetArcVertices로 삼각형 리스트
// (카메라를 향하는 대시 쿼드들, 월드 좌표)를 밀어 넣고, 비어 있으면 그리지 않는다.
// 1px LINELIST는 시선 방향 궤적이 원근 압축으로 안 보여서 두께 있는 쿼드로 그린다.
class GrenadeArcPass : public IRenderPass {
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

	// 씬 → 패스 데이터 전달 (TRIANGLELIST 정점, 월드 좌표 cm, 3개 단위). 빈 벡터 = 숨김.
	static void SetArcVertices(std::vector<Vector3>&& v3Vertices) { s_v3ArcVertices = std::move(v3Vertices); }

private:
	ComPtr<ID3D12PipelineState> CreateLinePipelineState();

private:
	ComPtr<ID3D12PipelineState> m_pd3dArcPSO = nullptr;

	inline static std::vector<Vector3> s_v3ArcVertices;
};
