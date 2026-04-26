#include "pch.h"
#include "RenderGraph.h"
#include "GBufferPass.h"
#include "DefferedLightingPass.h"
#include "TransparentForwardLightingPass.h"
#include "DirectionalCascadeShadowMapPass.h"
#include "ToneMappingPass.h"
#include "SkyboxPass.h"
#include "SpritePass.h"
#include "DefferedFogPass.h"
#include <queue>

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
	
	std::shared_ptr<IRenderPass> pDefferedLightingPass = std::make_shared<DefferedLightingPass>();
	pDefferedLightingPass->Initialize();
	m_pAdjLists.push_back(pDefferedLightingPass);
	
	std::shared_ptr<IRenderPass> pDefferedFogPass = std::make_shared<DefferedFogPass>();
	pDefferedFogPass->Initialize();
	m_pAdjLists.push_back(pDefferedFogPass);
	
	std::shared_ptr<IRenderPass> pTransparentForwardPass = std::make_shared<TransparentForwardLightingPass>();
	pTransparentForwardPass->Initialize();
	m_pAdjLists.push_back(pTransparentForwardPass);

	std::shared_ptr<IRenderPass> pSkyboxPass = std::make_shared<SkyboxPass>();
	pSkyboxPass->Initialize();
	m_pAdjLists.push_back(pSkyboxPass);

	std::shared_ptr<IRenderPass> pToneMappingPass = std::make_shared<ToneMappingPass>();
	pToneMappingPass->Initialize();
	m_pAdjLists.push_back(pToneMappingPass);

	std::shared_ptr<IRenderPass> pSpritePass = std::make_shared<SpritePass>();
	pSpritePass->Initialize();
	m_pAdjLists.push_back(pSpritePass);
	
	pDirectionalCascadeShadowMapPass->Connect(pGBufferPass);
	pGBufferPass->Connect(pDefferedLightingPass);
	pDefferedLightingPass->Connect(pDefferedFogPass);
	pDefferedFogPass->Connect(pTransparentForwardPass);
	pTransparentForwardPass->Connect(pSkyboxPass);
	pSkyboxPass->Connect(pToneMappingPass);
	pToneMappingPass->Connect(pSpritePass);

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
