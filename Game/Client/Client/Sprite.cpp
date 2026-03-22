#include "pch.h"
#include "Sprite.h"
#include "Texture.h"

Sprite::Sprite(float fLeft, float fTop, float fRight, float fBottom, UINT uiLayerIndex, bool bClickable)
{
	m_Rect.fLeft = fLeft;
	m_Rect.fTop = fTop;
	m_Rect.fRight = fRight;
	m_Rect.fBottom = fBottom;

	m_bClickable = bClickable;

	m_nLayerIndex = uiLayerIndex;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TexturedSprite

TexturedSprite::TexturedSprite(const std::string& strTextureName, float fLeft, float fTop, float fRight, float fBottom, UINT uiLayerIndex, bool bClickable)
	: Sprite(fLeft, fTop, fRight, fBottom, uiLayerIndex, bClickable)
{
	m_pTexture = TEXTURE->GetTextureByName(strTextureName, TEXTURE_RESOURCE_TYPE::SRV);
}

void TexturedSprite::SetTexture(std::shared_ptr<Texture> pTexture)
{
	m_pTexture = pTexture;
}

void BillboardSprite::SetPosition(XMFLOAT3 xmf3Position)
{
	m_xmf3Position = xmf3Position;
}

void BillboardSprite::SetSize(XMFLOAT2 xmf2Size)
{
	m_xmf2Size = xmf2Size;
}

void Sprite::CreateShaderVariables(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList)
{
}

void TexturedSprite::AddToUI(UINT nLayerIndex)
{
	UI->Add(shared_from_this(), SPRITE_TYPE_TEXTURE, nLayerIndex);
}

void TexturedSprite::Render(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& descHandle) const
{
}

void Sprite::SetLayerIndex(UINT uiLayerIndex)
{
	m_nLayerIndex = uiLayerIndex;
}

bool Sprite::IsCursorInSprite(float x, float y) const
{
	if (!m_bClickable) {
		return false;
	}

	if (x >= m_Rect.fLeft && x <= m_Rect.fRight &&
		y >= m_Rect.fTop && y <= m_Rect.fBottom) {
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TextSprite

TextSprite::TextSprite(const std::string& strText, float fLeft, float fTop, float fRight, float fBottom, XMFLOAT4 xmf4TextColor, UINT uiLayerIndex, bool bClickable)
	: Sprite(fLeft, fTop, fRight, fBottom, uiLayerIndex, bClickable)
{
	assert(strText.length() <= MAX_CHARACTER_PER_SPRITE);
	const char* cstrText = strText.c_str();
	strcpy_s(m_cstrText, strText.length() + 1, cstrText);	// NULL 문자 포함
	m_nTextLength = strText.length();

	m_xmf4TextColor = xmf4TextColor;
}

void TextSprite::SetText(const std::string& strText)
{
	assert(strText.length() < MAX_CHARACTER_PER_SPRITE);
	const char* cstrText = strText.c_str();
	strcpy_s(m_cstrText, strText.length() + 1, cstrText);	// NULL 문자 포함
	m_nTextLength = strText.length();
}

void TextSprite::SetTextColor(const XMFLOAT4& xmf4TextColor)
{
	m_xmf4TextColor = xmf4TextColor;
}

void TextSprite::CreateShaderVariables(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList)
{
	Sprite::CreateShaderVariables(pd3dDevice, pd3dCommandList);
}

void TextSprite::AddToUI(UINT nLayerIndex)
{
	UI->Add(shared_from_this(), SPRITE_TYPE_TEXT, nLayerIndex);
}

void TextSprite::Render(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& descHandle) const
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BillboardSprite

BillboardSprite::BillboardSprite(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const std::string& strTextureName, XMFLOAT3 xmf3Position, XMFLOAT2 xmf2Size)
	: Sprite(0, 0, 0, 0, 0, false)
{
	m_pTexture = TEXTURE->GetTextureByName(strTextureName, TEXTURE_RESOURCE_TYPE::SRV);
	m_xmf3Position = xmf3Position;
	m_xmf2Size = xmf2Size;
}

void BillboardSprite::AddToUI(UINT nLayerIndex)
{
	UI->Add(shared_from_this(), SPRITE_TYPE_BILLBOARD, 0);
}

void BillboardSprite::Render(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& descHandle) const
{
}
