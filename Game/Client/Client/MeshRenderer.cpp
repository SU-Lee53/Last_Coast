#include "pch.h"
#include "MeshRenderer.h"

uint64_t IMeshRenderer::g_ui64RendererIDBase = 0;

IMeshRenderer::IMeshRenderer()
{
	m_ui64RendererID = ++g_ui64RendererIDBase;
}

IMeshRenderer::IMeshRenderer(const IMeshRenderer& other)
{
	*this = other;
}

IMeshRenderer::IMeshRenderer(IMeshRenderer&& other)
{
	*this = std::forward<IMeshRenderer>(other);
}

IMeshRenderer& IMeshRenderer::operator=(const IMeshRenderer& other)
{
	if (this == &other) {
		return *this;
	}

	m_pMeshes = other.m_pMeshes;
	m_pMaterials = other.m_pMaterials;

	return *this;
}

IMeshRenderer& IMeshRenderer::operator=(IMeshRenderer&& other)
{
	if (this == &other) {
		return *this;
	}

	m_pMeshes = std::move(other.m_pMeshes);
	m_pMaterials = std::move(other.m_pMaterials);

	return *this;
}

bool IMeshRenderer::operator==(const IMeshRenderer& rhs) const
{
	return m_ui64RendererID == rhs.m_ui64RendererID;
}

BoundingOrientedBox IMeshRenderer::GetOBBMerged() const
{
	BoundingBox xmAABBMerged{};
	for (const auto& pMesh : m_pMeshes) {
		XMFLOAT3 pxmf3OBBPoints[BoundingOrientedBox::CORNER_COUNT];
		pMesh->GetBoundingBox().GetCorners(pxmf3OBBPoints); 
		BoundingBox xmAABB{};
		BoundingBox::CreateFromPoints(xmAABB, BoundingOrientedBox::CORNER_COUNT, pxmf3OBBPoints, sizeof(XMFLOAT3));

		BoundingBox::CreateMerged(xmAABBMerged, xmAABBMerged, xmAABB);
	}

	BoundingOrientedBox xmOBBResult{};
	BoundingOrientedBox::CreateFromBoundingBox(xmOBBResult, xmAABBMerged);

	return xmOBBResult;
}

void IMeshRenderer::SetTexture(std::shared_ptr<Texture> pTexture, UINT nMaterialIndex, TEXTURE_TYPE eTextureType)
{
	assert(pTexture);
	m_pMaterials[nMaterialIndex]->SetTexture(pTexture, eTextureType);
}
