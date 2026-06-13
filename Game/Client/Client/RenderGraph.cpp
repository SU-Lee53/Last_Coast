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
#include "BoundingBoxDebugPass.h"
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
	// HDR[1] : DefferedForPass -> TransparentForwardPass -> SkyboxPass
	// SkyBoxPass 를 지나면 HDR1 이 gtxtHDRResult(Root Param 4) 에 바인딩


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

	std::shared_ptr<IRenderPass> pParticlePass = std::make_shared<ParticlePass>();
	pParticlePass->Initialize();
	m_pAdjLists.push_back(pParticlePass);

	std::shared_ptr<BloomPass> pBloomPass = std::make_shared<BloomPass>();
	pBloomPass->Initialize();
	m_pAdjLists.push_back(pBloomPass);

	std::shared_ptr<AutoExposurePass> pAutoExposurePass = std::make_shared<AutoExposurePass>();
	pAutoExposurePass->Initialize();
	m_pAdjLists.push_back(pAutoExposurePass);

	std::shared_ptr<IRenderPass> pLightShaftPass = std::make_shared<LightShaftPass>();
	pLightShaftPass->Initialize();
	m_pAdjLists.push_back(pLightShaftPass);

	std::shared_ptr<IRenderPass> pSkyboxPass = std::make_shared<SkyboxPass>();
	pSkyboxPass->Initialize();
	m_pAdjLists.push_back(pSkyboxPass);

	std::shared_ptr<IRenderPass> pToneMappingPass = std::make_shared<ToneMappingPass>();
	pToneMappingPass->Initialize();
	m_pAdjLists.push_back(pToneMappingPass);

	std::shared_ptr<IRenderPass> pBoundingBoxDebugPass = std::make_shared<BoundingBoxDebugPass>();
	pBoundingBoxDebugPass->Initialize();
	m_pAdjLists.push_back(pBoundingBoxDebugPass);

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
		
		+ 2026.05.16
		+ Bloom
		
		+ 2026.05.22
		+ Auto Exposure luminance

		8. Tone Mapping & Grading pass
		9. UI pass
	*/

	pDirectionalCascadeShadowMapPass->Connect(pGBufferPass);
	pGBufferPass->Connect(pSSAOPass);
	pSSAOPass->Connect(pDefferedLightingPass);
	pDefferedLightingPass->Connect(pDefferedFogPass);
	pDefferedFogPass->Connect(pTransparentForwardPass);
	pTransparentForwardPass->Connect(pSkyboxPass);
	pSkyboxPass->Connect(pLightShaftPass);
	pLightShaftPass->Connect(pParticlePass);
	pParticlePass->Connect(pBloomPass);
	pBloomPass->Connect(pAutoExposurePass);
	pAutoExposurePass->Connect(pToneMappingPass);
	pToneMappingPass->Connect(pBoundingBoxDebugPass);
	pBoundingBoxDebugPass->Connect(pUIPass);

	m_unEntryNodeIndex = 0;
}

void RenderGraph::Run(OUT DescriptorHandle& outDescHandle, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, void* pAdditionalContext)
{
	std::queue<std::shared_ptr<IRenderPass>> pBFSQueue;
	pBFSQueue.push(m_pAdjLists[m_unEntryNodeIndex]);

	RenderPassInput passInput = input;

	while (pBFSQueue.size() != 0) {
		// 1. Pop
		const std::shared_ptr<IRenderPass> pCurPass = pBFSQueue.front();
		pBFSQueue.pop();
		
		// 2. Run
		RenderPassOutput passOutput;
		pCurPass->Execute(pd3dCommandList, passInput, passOutput, outDescHandle);

		passInput = passOutput.ToInput();

		// 3. Push queue
		const auto& pEdges = pCurPass->GetEdges();
		for (auto& pEdge : pEdges) {
			pBFSQueue.push(pEdge);
		}
	}
}

void RenderGraph::ShowDebugInfo() const
{
	for (const auto& pPass : m_pAdjLists) {
		if (ImGui::TreeNode(typeid(*pPass).name())) {
			pPass->ShowDebugInfo();
			ImGui::TreePop();
		}
	}
}
