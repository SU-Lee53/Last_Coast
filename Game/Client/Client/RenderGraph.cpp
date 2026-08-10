#include "pch.h"
#include "RenderGraph.h"
#include "GBufferPass.h"
#include "SSAOPass.h"
#include "DeferredLightingPass.h"
#include "TransparentForwardLightingPass.h"
#include "DirectionalCascadeShadowMapPass.h"
#include "ToneMappingPass.h"
#include "SkyboxPass.h"
#include "ParticlePass.h"
#include "BloomPass.h"
#include "AutoExposurePass.h"
#include "LightShaftPass.h"
#include "DeferredFogPass.h"
#include "AtmosphericFogDetailPass.h"
#include "BoundingBoxDebugPass.h"
#include "NavMeshDebugPass.h"
#include "GrenadeArcPass.h"
#include "UIPass.h"

void RenderGraph::BuildGraph()
{
	//std::shared_ptr<IRenderPass> pForwardPass = std::make_shared<ForwardPass>();
	//pForwardPass->Initialize();
	//m_pAdjLists.push_back(pForwardPass);
	//
	//if (CUR_SCENE->GetTerrain()) {
	//	std::shared_ptr<IRenderPass> pTerrainPass = std::make_shared<TerrainPass>();
	//	pTerrainPass->Initialize();
	//	pForwardPass->Connect(pTerrainPass);
	//	m_pAdjLists.push_back(pTerrainPass);
	//}
	
	// Ping-Pong 구조
	// HDR[0] : GBufferPass -> DefferedLightingPass
	// DefferedLightingPass 를 지나면 HDR0 이 gtxtHDRResult(Root Param 4) 에 바인딩
	// 
	// HDR[1] : DefferedFogPass -> TransparentForwardPass -> SkyboxPass -> ParticlePass
	// AtmosphericFogDetailPass 는 HDR0 에 결과를 쓴 뒤 HDR1 로 복사해 후속 패스 입력을 유지


	std::shared_ptr<IRenderPass> pDirectionalCascadeShadowMapPass = std::make_shared<DirectionalCascadeShadowMapPass>();
	pDirectionalCascadeShadowMapPass->Initialize();
	m_pAdjLists.push_back(pDirectionalCascadeShadowMapPass);
	
	std::shared_ptr<IRenderPass> pGBufferPass = std::make_shared<GBufferPass>();
	pGBufferPass->Initialize();
	m_pAdjLists.push_back(pGBufferPass);
	
	std::shared_ptr<IRenderPass> pSSAOPass = std::make_shared<SSAOPass>();
	pSSAOPass->Initialize();
	m_pAdjLists.push_back(pSSAOPass);
	
	std::shared_ptr<IRenderPass> pDefferedLightingPass = std::make_shared<DeferredLightingPass>();
	pDefferedLightingPass->Initialize();
	m_pAdjLists.push_back(pDefferedLightingPass);
	
	std::shared_ptr<IRenderPass> pDefferedFogPass = std::make_shared<DeferredFogPass>();
	pDefferedFogPass->Initialize();
	m_pAdjLists.push_back(pDefferedFogPass);
	
	std::shared_ptr<IRenderPass> pTransparentForwardPass = std::make_shared<TransparentForwardLightingPass>();
	pTransparentForwardPass->Initialize();
	m_pAdjLists.push_back(pTransparentForwardPass);

	std::shared_ptr<IRenderPass> pSkyboxPass = std::make_shared<SkyboxPass>();
	pSkyboxPass->Initialize();
	m_pAdjLists.push_back(pSkyboxPass);

	std::shared_ptr<IRenderPass> pLightShaftPass = std::make_shared<LightShaftPass>();
	pLightShaftPass->Initialize();
	m_pAdjLists.push_back(pLightShaftPass);

	std::shared_ptr<IRenderPass> pParticlePass = std::make_shared<ParticlePass>();
	pParticlePass->Initialize();
	m_pAdjLists.push_back(pParticlePass);

	std::shared_ptr<IRenderPass> pAtmosphericFogDetailPass = std::make_shared<AtmosphericFogDetailPass>();
	pAtmosphericFogDetailPass->Initialize();
	m_pAdjLists.push_back(pAtmosphericFogDetailPass);

	std::shared_ptr<BloomPass> pBloomPass = std::make_shared<BloomPass>();
	pBloomPass->Initialize();
	m_pAdjLists.push_back(pBloomPass);

	std::shared_ptr<AutoExposurePass> pAutoExposurePass = std::make_shared<AutoExposurePass>();
	pAutoExposurePass->Initialize();
	m_pAdjLists.push_back(pAutoExposurePass);

	std::shared_ptr<IRenderPass> pToneMappingPass = std::make_shared<ToneMappingPass>();
	pToneMappingPass->Initialize();
	m_pAdjLists.push_back(pToneMappingPass);

	std::shared_ptr<IRenderPass> pBoundingBoxDebugPass = std::make_shared<BoundingBoxDebugPass>();
	pBoundingBoxDebugPass->Initialize();
	m_pAdjLists.push_back(pBoundingBoxDebugPass);

	std::shared_ptr<IRenderPass> pNavMeshDebugPass = std::make_shared<NavMeshDebugPass>();
	pNavMeshDebugPass->Initialize();
	m_pAdjLists.push_back(pNavMeshDebugPass);

	std::shared_ptr<IRenderPass> pGrenadeArcPass = std::make_shared<GrenadeArcPass>();
	pGrenadeArcPass->Initialize();
	m_pAdjLists.push_back(pGrenadeArcPass);

	std::shared_ptr<IRenderPass> pUIPass = std::make_shared<UIPass>();
	pUIPass->Initialize();
	m_pAdjLists.push_back(pUIPass);

	/*
		1. Dirctional CSM pass
		2. GBuffer pass

		+ 2026.05.21
		+ SSAO

		3. Deffered Lighting pass
		4. Deffered Fog pass
		5. Forward Lighting pass
		6. Skybox pass
		7. Particle pass
		8. Atmospheric Fog Detail pass
		
		+ 2026.05.16
		+ Bloom
		
		+ 2026.05.22
		+ Auto Exposure luminance

		9. Tone Mapping & Grading pass
		10. UI pass
	*/

	pDirectionalCascadeShadowMapPass->Connect(pGBufferPass);
	pGBufferPass->Connect(pSSAOPass);
	pSSAOPass->Connect(pDefferedLightingPass);
	pDefferedLightingPass->Connect(pDefferedFogPass);
	pDefferedFogPass->Connect(pTransparentForwardPass);
	pTransparentForwardPass->Connect(pSkyboxPass);
	pSkyboxPass->Connect(pLightShaftPass);
	pLightShaftPass->Connect(pParticlePass);
	pParticlePass->Connect(pAtmosphericFogDetailPass);
	pAtmosphericFogDetailPass->Connect(pBloomPass);
	pBloomPass->Connect(pAutoExposurePass);
	pAutoExposurePass->Connect(pToneMappingPass);
	pToneMappingPass->Connect(pBoundingBoxDebugPass);
	pBoundingBoxDebugPass->Connect(pNavMeshDebugPass);
	pNavMeshDebugPass->Connect(pGrenadeArcPass);
	pGrenadeArcPass->Connect(pUIPass);

	m_unEntryNodeIndex = 0;
	m_llPassTime.resize(m_pAdjLists.size(), 0);
}

void RenderGraph::Clear()
{
	m_OutputCache = {};
	m_pAdjLists.clear();
	m_llPassTime.clear();
	m_unEntryNodeIndex = 0;
	m_fLastRecordTime = 0.f;
}

void RenderGraph::Run(OUT DescriptorHandle& outDescHandle, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, void* pAdditionalContext)
{
	using namespace std::chrono;

	m_fLastRecordTime += DT;
	std::queue<std::shared_ptr<IRenderPass>> pBFSQueue;
	pBFSQueue.push(m_pAdjLists[m_unEntryNodeIndex]);

	RenderPassInput passInput = input;


	for (uint32 i = 0; i < m_pAdjLists.size(); ++i) {
		auto beginTime = high_resolution_clock::now();
		RenderPassOutput passOutput;

		// PIX ANSI 마커(version=1) — DRED breadcrumb 컨텍스트로 기록되어 device removed 시 hang 패스 식별에 쓰인다
		const char* pszPassName = typeid(*m_pAdjLists[i]).name();
		pd3dCommandList->BeginEvent(1, pszPassName, static_cast<UINT>(::strlen(pszPassName) + 1));

		m_pAdjLists[i]->Execute(pd3dCommandList, passInput, passOutput, outDescHandle);

		pd3dCommandList->EndEvent();

		passInput = passOutput.ToInput();
		auto endTime = high_resolution_clock::now();

		if (m_fLastRecordTime >= m_fRecordInterval) {
			m_llPassTime[i] = duration_cast<microseconds>(endTime - beginTime).count();
		}
	}

	if (m_fLastRecordTime >= m_fRecordInterval) {
		m_fLastRecordTime = 0.f;
	}

	//while (pBFSQueue.size() != 0) {
	//	// 1. Pop
	//	const std::shared_ptr<IRenderPass> pCurPass = pBFSQueue.front();
	//	pBFSQueue.pop();
	//	
	//	// 2. Run
	//	RenderPassOutput passOutput;
	//	pCurPass->Execute(pd3dCommandList, passInput, passOutput, outDescHandle);
	//
	//	passInput = passOutput.ToInput();
	//
	//	// 3. Push queue
	//	const auto& pEdges = pCurPass->GetEdges();
	//	for (auto& pEdge : pEdges) {
	//		pBFSQueue.push(pEdge);
	//	}
	//}
}

void RenderGraph::ShowDebugInfo() const
{
	std::string strPassNames[] = {
		"DirectionalCascadeShadowMapPass",
		"GBufferPass",
		"SSAOPass",
		"DefferedLightingPass",
		"DefferedFogPass",
		"TransparentForwardPass",
		"SkyboxPass",
		"LightShaftPass",
		"ParticlePass",
		"AtmosphericFogDetailPass",
		"BloomPass",
		"AutoExposurePass",
		"ToneMappingPass",
		"BoundingBoxDebugPass",
		"NavMeshDebugPass",
		"GrenadeArcPass",
		"pUIPass"
	};

	long long totalTime = std::accumulate(m_llPassTime.begin(), m_llPassTime.end(), 0LL);
	ImGui::Text("Total : %lld", totalTime);
	for (uint32 i = 0; i < m_llPassTime.size(); ++i) {
		const float fRatio = (totalTime > 0) ? ((float)m_llPassTime[i] / (float)totalTime) * 100.0f : 0.0f;
		ImGui::Text("%s : Time : %lld us (%f)", strPassNames[i].c_str(), m_llPassTime[i], fRatio);
	}

	for (const auto& pPass : m_pAdjLists) {
		if (ImGui::TreeNode(typeid(*pPass).name())) {
			pPass->ShowDebugInfo();
			ImGui::TreePop();
		}
	}
}
