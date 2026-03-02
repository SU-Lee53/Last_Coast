#pragma once
#include "RenderPass.h"

class RenderGraph {
public:
	void BuildGraph();
	void Run(
		OUT DescriptorHandle& outDescHandle, 
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		const RenderPassInput& input, 
		void* pAdditionalContext = nullptr);

private:
	std::vector<std::shared_ptr<IRenderPass>> m_pAdjLists;
	uint32 m_unEntryNodeIndex = 0;

	RenderPassOutput m_OutputCache{};

};

