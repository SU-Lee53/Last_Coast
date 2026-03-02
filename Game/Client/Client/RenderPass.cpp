#include "pch.h"
#include "RenderPass.h"
#include "TerrainObject.h"

void IRenderPass::Connect(std::shared_ptr<IRenderPass> pNode)
{
	m_pEdgeList.push_back(pNode);
}
