#pragma once
#include "TextAtlas.h"

struct FontDesc {
	std::wstring wstrFamilyName;
	float fFontSize;
	DWRITE_FONT_WEIGHT fontWeight = DWRITE_FONT_WEIGHT_NORMAL;
	DWRITE_FONT_STYLE fontStyle = DWRITE_FONT_STYLE_NORMAL;
	DWRITE_FONT_STRETCH fontStretch = DWRITE_FONT_STRETCH_NORMAL;
};

struct Font {
	using ID = uint64_t;

	FontDesc desc;
	ComPtr<IDWriteTextFormat> pdwTextFormat = nullptr;
	bool bValid = false;
};

struct TextLayout {
	ComPtr<IDWriteTextLayout> pdwTextLayout = nullptr;
	float fWidth = 0.f;
	float fHeight = 0.f;
};

struct TextCacheKey {
	Font::ID fontID = INVALID_ID;
	std::wstring wstrText;

	bool operator==(const TextCacheKey& rhs) const {
		return fontID == rhs.fontID && wstrText == rhs.wstrText;
	}
};

struct TextCacheKeyHasher {
	size_t operator()(const TextCacheKey& key) const {
		size_t h1 = std::hash<uint64_t>{}(key.fontID);
		size_t h2 = std::hash<std::wstring>{}(key.wstrText);
		return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
	}
};

struct CachedText {
	Font::ID fontID = INVALID_ID;
	std::wstring wstrText;

	TextAtlas::AtlasRect Rect{};

	bool bValid = false;
	bool bDirty = true;
};

using CachedTextTable = ResourceTable<TextCacheKey, CachedText, TextCacheKeyHasher>;
using TextHandle = ResourceHandle<TextCacheKey, CachedText, TextCacheKeyHasher>;

class TextManager {

	DECLARE_SINGLE(TextManager);

public:
	void Initialize(const ComPtr<ID3D12CommandQueue>& pd3dCommandQueue);
	TextHandle GetOrCacheText(Font::ID fontID, const std::wstring& wstrText);


	const ComPtr<ID3D11On12Device>& GetD3D11On12Device() const { return m_pd3d11On12Device; };
	const ComPtr<ID3D11DeviceContext>& GetD3D11DeviceContext() const { return m_pd3d11DeviceContext; };
	const ComPtr<ID2D1Device2>& GetD2DDevice() const { return m_pd2dDevice; };
	const ComPtr<ID2D1DeviceContext2>& GetD2DDeviceContext() const { return m_pd2dDeviceContext; };
	const ComPtr<ID2D1SolidColorBrush>& GetBrush() const { return m_pBrush; };

	float GetDpiX() const { return m_fDpiX; }
	float GetDpiY() const { return m_fDpiY; }

public:
	// Font handling
	Font::ID RegisterFont(const FontDesc& desc);
	void DestroyFont(const Font::ID& id);
	const Font* GetFont(Font::ID id) const;

private:
	HRESULT CreateTextLayout(
		Font::ID fontID,
		const std::wstring& wstrText,
		float fMaxWidth,
		float fMaxHeight,
		OUT TextLayout& outLayout);

	HRESULT CacheText(
		Font::ID fontID,
		const wstring& wstrText,
		OUT CachedText& outCachedText);

	HRESULT UpdateCachedText(OUT CachedText& cachedText);

	// Text Atlas
	HRESULT CreateMainAtlas(uint32 unWidth, uint32 unHeight);
	HRESULT RebuildAtlas();

private:
	// DPI : Dots Per Inch 
	float m_fDpiX = 96.f;
	float m_fDpiY = 96.f;

	// Fonts
	std::vector<Font> m_Fonts;

	// Text atlas
	TextAtlas m_TextAtlas;

	// Text Cache
	//std::vector<CachedText> m_CachedText;
	//IndexMap<TextCacheKey, CachedText, TextCacheKeyHasher> m_CachedTextMap;
	CachedTextTable m_TextTable{};

private:
	// Internal DirectX Components
	// D3D11On12
	ComPtr<ID3D11Device>			m_pd3d11Device = nullptr;
	ComPtr<ID3D11DeviceContext>		m_pd3d11DeviceContext = nullptr;
	ComPtr<ID3D11On12Device>		m_pd3d11On12Device = nullptr;

	// Command queue ref in RenderManager
	ComPtr<ID3D12CommandQueue>		m_pd3dCommandQueue = nullptr;

	// D2D1
	ComPtr<ID2D1Factory3>			m_pd2dFactory = nullptr;
	ComPtr<ID2D1Device2>			m_pd2dDevice = nullptr;
	ComPtr<ID2D1DeviceContext2>		m_pd2dDeviceContext = nullptr;

	// DirectWrite
	ComPtr<IDWriteFactory5>			m_pdwFactory = nullptr;

	// Brush
	ComPtr<ID2D1SolidColorBrush>	m_pBrush = nullptr;

	// Initializer
	HRESULT InitializeD3D11On12(const ComPtr<ID3D12CommandQueue>& pd3dCommandQueue);
	HRESULT InitializeD2D1();
	HRESULT InitializeDirectWrite();
	HRESULT CreateSharedResources();	// Create D2D1 <-> D3D shared resources

};
