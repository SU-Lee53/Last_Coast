#include "pch.h"
#include "RenderGraph.h"
#include "ForwardPass.h"
#include "TerrainPass.h"
#include "GBufferPass.h"
#include "ToneMappingPass.h"
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
	
	std::shared_ptr<IRenderPass> pGBufferPass = std::make_shared<GBufferPass>();
	pGBufferPass->Initialize();
	m_pAdjLists.push_back(pGBufferPass);
	
	std::shared_ptr<IRenderPass> pLightingPass = std::make_shared<DefferedLightingPass>();
	pLightingPass->Initialize();
	m_pAdjLists.push_back(pLightingPass);
	
	std::shared_ptr<IRenderPass> pToneMappingPass = std::make_shared<ToneMappingPass>();
	pToneMappingPass->Initialize();
	m_pAdjLists.push_back(pToneMappingPass);

	pGBufferPass->Connect(pLightingPass);
	pLightingPass->Connect(pToneMappingPass);

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
