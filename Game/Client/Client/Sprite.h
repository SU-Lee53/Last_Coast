#pragma once

#define MAX_CHARACTER_PER_SPRITE 40

enum SPRITE_TYPE : UINT8 {
	SPRITE_TYPE_TEXTURE = 0,
	SPRITE_TYPE_TEXT,
	SPRITE_TYPE_BILLBOARD,

	SPRITE_TYPE_COUNT
};

struct SpriteRect {
	float fLeft;
	float fTop;
	float fRight;
	float fBottom;
};

struct CB_SPRITE_DATA {
	float fLeft;
	float fTop;
	float fRight;
	float fBottom;
};

class Sprite : public std::enable_shared_from_this<Sprite> {
public:
	Sprite(float fLeft, float fTop, float fRight, float fBottom, UINT uiLayerIndex = 0, bool bClickable = false);
	virtual ~Sprite() {};

	virtual void CreateShaderVariables(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList);
	virtual void AddToUI(UINT nLayerIndex) = 0;
	virtual void Render(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& descHandle) const = 0;

	void SetLayerIndex(UINT uiLayerIndex);
	UINT GetLayerIndex() const { return m_nLayerIndex; };

	bool IsCursorInSprite(float x, float y) const;

protected:
	SpriteRect m_Rect;	// 윈도우 좌표계를 기준
	bool m_bClickable;
	UINT m_nLayerIndex;

};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TexturedSprite

class Texture;

class TexturedSprite : public Sprite {
public:
	TexturedSprite(const std::string& strTextureName, float fLeft, float fTop, float fRight, float fBottom, UINT uiLayerIndex = 0, bool bClickable = false);
	virtual ~TexturedSprite() {};

	void SetTexture(std::shared_ptr<Texture> pTexture);

	virtual void AddToUI(UINT nLayerIndex) override;
	virtual void Render(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& descHandle) const override;

private:
	std::shared_ptr<Texture> m_pTexture;

};


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TextSprite

struct CB_TEXT_DATA {
	UINT nCharacters[MAX_CHARACTER_PER_SPRITE];
	XMFLOAT4 xmf4TextColor;
	UINT nLength;
};

class TextSprite : public Sprite {
public:
	TextSprite(const std::string& strText, float fLeft, float fTop, float fRight, float fBottom, XMFLOAT4 xmf4TextColor = XMFLOAT4(1, 1, 1, 1), UINT uiLayerIndex = 0, bool bClickable = false);
	virtual ~TextSprite() {};

	void SetText(const std::string& strText);
	void SetTextColor(const XMFLOAT4& xmf4TextColor);

	virtual void CreateShaderVariables(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList) override;
	virtual void AddToUI(UINT nLayerIndex) override;
	virtual void Render(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& descHandle) const override;

private:
	char m_cstrText[MAX_CHARACTER_PER_SPRITE];
	int m_nTextLength = 0;
	XMFLOAT4 m_xmf4TextColor{ 1.f, 1.f, 1.f, 1.f };

};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BillboardSprite

struct CB_BILLBOARD_SPRITE_DATA {
	XMFLOAT3 xmf3Position;
	UINT pad1 = 0;
	XMFLOAT2 xmf2Size;
	XMUINT2 pad2 = XMUINT2(0,0);
	XMFLOAT3 xmf3CameraPosition;
	UINT pad3 = 0;
	XMFLOAT4X4 xmf4x4ViewProjection;
};

class BillboardSprite : public Sprite {
public:
	BillboardSprite(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const std::string& strTextureName, XMFLOAT3 xmf3Position, XMFLOAT2 xmf2Size);
	virtual ~BillboardSprite() {};

	void SetTexture(std::shared_ptr<Texture> pTexture);
	void SetPosition(XMFLOAT3 xmf3Position);
	void SetSize(XMFLOAT2 xmf2Size);

	virtual void AddToUI(UINT nLayerIndex) override;
	virtual void Render(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& descHandle) const override;

private:
	std::shared_ptr<Texture> m_pTexture;
	XMFLOAT3 m_xmf3Position;
	XMFLOAT2 m_xmf2Size;
};

