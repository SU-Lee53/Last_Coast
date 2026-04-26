#pragma once

struct TextLayout;

class TextAtlas {
public:
	friend class TextRenderer;

	class AtlasRect {
	public:
		AtlasRect() = default;
		AtlasRect(uint32 _x, uint32 _y, uint32 _w, uint32 _h) : x{ _x }, y{ _y }, w{ _w }, h{ _h } {}

		AtlasRect(const AtlasRect& r);
		AtlasRect(AtlasRect&& r) noexcept;

		AtlasRect& operator=(const AtlasRect& r);
		AtlasRect& operator=(AtlasRect&& r) noexcept;

		bool operator==(const AtlasRect& rhs) const;

		uint32 x = 0;
		uint32 y = 0;
		uint32 w = 0;
		uint32 h = 0;
	};

	enum class RebuildAlert { OK, NOT_ENOUGH_SPACE, SEVERE_FRAGMENTATION, INVALID_REQUEST };

public:
	HRESULT Initialize(uint32 unWidth, uint32 unHeight, uint32 unPadding = 2);
	HRESULT DrawTextToAtlas(const TextLayout& layout, uint32 unAtlasX, uint32 unAtlasY);

	// Getter & Setter
	uint32 GetWidth() const { return m_unWidth; }
	uint32 GetHeight() const { return m_unHeight; }
	uint32 GetPadding() const { return m_unPadding; }

	const TextureRef<RenderTargetTexture>& GetRenderTarget() const { return m_RenderTarget; }
	const ComPtr<ID3D11Resource>& GetWrapped11Resource() const { return m_pWrapped11Resource; }
	const ComPtr<ID2D1Bitmap1>& GetD2DTargetBitmap() const { return m_pd2dTargetBitmap; }

	D3D12_CPU_DESCRIPTOR_HANDLE GetSRVHandle() { return m_srvCPUHandle; }

	const std::vector<AtlasRect>& GetFreeRects() const { return m_FreeRects; }

	// Atlas Management
	HRESULT Clear();

	bool AllocateAtlasRect(uint32 unWidth, uint32 unHeight, OUT AtlasRect& outRect);
	void FreeAtlasRect(const AtlasRect& rect);
	void MergeAtlasRect();
	RebuildAlert NeedsAtlasRebuild(uint32 unRequestWidth, uint32 unRequestHeight) const;
	void Reset();

private:
	uint32 m_unWidth = 0;
	uint32 m_unHeight = 0;
	uint32 m_unPadding = 2;

	TextureRef<RenderTargetTexture>	m_RenderTarget{};
	ComPtr<ID3D11Resource>			m_pWrapped11Resource = nullptr;
	ComPtr<ID2D1Bitmap1>			m_pd2dTargetBitmap = nullptr;

	D3D12_CPU_DESCRIPTOR_HANDLE		m_srvCPUHandle{};
	bool							m_bHasSRV = false;

	std::vector<AtlasRect> m_FreeRects;

};
