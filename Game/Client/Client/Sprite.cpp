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
	m_pTexture = TEXTURE->Get(strTextureName);
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
	ConstantBuffer& cbSprite = RESOURCE->AllocCBuffer<CB_SPRITE_DATA>();

	CB_SPRITE_DATA spriteData;
	{
		spriteData.fLeft = m_Rect.fLeft;
		spriteData.fTop = m_Rect.fTop;
		spriteData.fRight = m_Rect.fRight;
		spriteData.fBottom = m_Rect.fBottom;
	}
	cbSprite.WriteData(&spriteData);

	pd3dDevice->CopyDescriptorsSimple(1, descHandle.cpuHandle, cbSprite.CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	descHandle.cpuHandle.Offset(1, D3DCore::g_nCBVSRVDescriptorIncrementSize);

	pd3dCommandList->SetGraphicsRootDescriptorTable(1, descHandle.gpuHandle);
	descHandle.gpuHandle.Offset(1, D3DCore::g_nCBVSRVDescriptorIncrementSize);
	
	pd3dDevice->CopyDescriptorsSimple(1, descHandle.cpuHandle, m_pTexture->GetHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	descHandle.cpuHandle.Offset(1, D3DCore::g_nCBVSRVDescriptorIncrementSize);

	pd3dCommandList->SetGraphicsRootDescriptorTable(3, descHandle.gpuHandle);
	descHandle.gpuHandle.Offset(1, D3DCore::g_nCBVSRVDescriptorIncrementSize);

	pd3dCommandList->DrawInstanced(1, 1, 0, 0);
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
	ConstantBuffer& cbSprite = RESOURCE->AllocCBuffer<CB_SPRITE_DATA>();

	CB_SPRITE_DATA spriteData;
	{
		spriteData.fLeft = m_Rect.fLeft;
		spriteData.fTop = m_Rect.fTop;
		spriteData.fRight = m_Rect.fRight;
		spriteData.fBottom = m_Rect.fBottom;
	}
	cbSprite.WriteData(&spriteData);

	pd3dDevice->CopyDescriptorsSimple(1, descHandle.cpuHandle, cbSprite.CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	descHandle.cpuHandle.Offset(1, D3DCore::g_nCBVSRVDescriptorIncrementSize);

	pd3dCommandList->SetGraphicsRootDescriptorTable(1, descHandle.gpuHandle);
	descHandle.gpuHandle.Offset(1, D3DCore::g_nCBVSRVDescriptorIncrementSize);

	ConstantBuffer& cbText = RESOURCE->AllocCBuffer<CB_TEXT_DATA>();

	CB_TEXT_DATA textData;
	{
		memset(textData.nCharacters, 0, sizeof(textData.nCharacters));

		for (int i = 0; i < m_nTextLength; ++i) {
			textData.nCharacters[i] = m_cstrText[i];
		}
		//textData.pad = XMUINT2(99, 99);
		textData.nLength = m_nTextLength;
		textData.xmf4TextColor = m_xmf4TextColor;
	}
	cbText.WriteData(&textData);

	pd3dDevice->CopyDescriptorsSimple(1, descHandle.cpuHandle, cbText.CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	descHandle.cpuHandle.Offset(1, D3DCore::g_nCBVSRVDescriptorIncrementSize);

	pd3dCommandList->SetGraphicsRootDescriptorTable(2, descHandle.gpuHandle);
	descHandle.gpuHandle.Offset(1, D3DCore::g_nCBVSRVDescriptorIncrementSize);

	pd3dCommandList->DrawInstanced(m_nTextLength, 1, 0, 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BillboardSprite

BillboardSprite::BillboardSprite(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const std::string& strTextureName, XMFLOAT3 xmf3Position, XMFLOAT2 xmf2Size)
	: Sprite(0, 0, 0, 0, 0, false)
{
	m_pTexture = TEXTURE->Get(strTextureName);
	m_xmf3Position = xmf3Position;
	m_xmf2Size = xmf2Size;
}

void BillboardSprite::AddToUI(UINT nLayerIndex)
{
	UI->Add(shared_from_this(), SPRITE_TYPE_BILLBOARD, 0);
}

void BillboardSprite::Render(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& descHandle) const
{
	ConstantBuffer& cbBillboard = RESOURCE->AllocCBuffer<CB_BILLBOARD_SPRITE_DATA>();

	CB_BILLBOARD_SPRITE_DATA billboardData;
	{
		billboardData.xmf3Position = m_xmf3Position;
		billboardData.xmf2Size = m_xmf2Size;
		billboardData.xmf3CameraPosition = CUR_SCENE->GetCamera()->GetPosition();

		XMMATRIX xmmtxCameraView = XMLoadFloat4x4(&CUR_SCENE->GetCamera()->GetViewMatrix());
		XMMATRIX xmmtxCameraProjection = XMLoadFloat4x4(&CUR_SCENE->GetCamera()->GetProjectionMatrix());
		XMStoreFloat4x4(&billboardData.xmf4x4ViewProjection, XMMatrixTranspose(XMMatrixMultiply(xmmtxCameraView, xmmtxCameraProjection)));
	}
	cbBillboard.WriteData(&billboardData);

	pd3dDevice->CopyDescriptorsSimple(1, descHandle.cpuHandle, cbBillboard.CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	descHandle.cpuHandle.Offset(1, D3DCore::g_nCBVSRVDescriptorIncrementSize);

	pd3dCommandList->SetGraphicsRootDescriptorTable(4, descHandle.gpuHandle);
	descHandle.gpuHandle.Offset(1, D3DCore::g_nCBVSRVDescriptorIncrementSize);

	pd3dDevice->CopyDescriptorsSimple(1, descHandle.cpuHandle, m_pTexture->GetHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	descHandle.cpuHandle.Offset(1, D3DCore::g_nCBVSRVDescriptorIncrementSize);

	pd3dCommandList->SetGraphicsRootDescriptorTable(3, descHandle.gpuHandle);
	descHandle.gpuHandle.Offset(1, D3DCore::g_nCBVSRVDescriptorIncrementSize);

	pd3dCommandList->DrawInstanced(1, 1, 0, 0);
}
