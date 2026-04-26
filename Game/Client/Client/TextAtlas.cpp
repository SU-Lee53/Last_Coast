#include "pch.h"
#include "TextAtlas.h"

TextAtlas::AtlasRect::AtlasRect(const AtlasRect& r)
{
	*this = r;
}

TextAtlas::AtlasRect::AtlasRect(AtlasRect&& r) noexcept
{
	*this = std::move(r);
}

TextAtlas::AtlasRect& TextAtlas::AtlasRect::operator=(const AtlasRect& r)
{
	if (this == &r) {
		return *this;
	}

	x = r.x;
	y = r.y;
	w = r.w;
	h = r.h;
	return *this;
}

TextAtlas::AtlasRect& TextAtlas::AtlasRect::operator=(AtlasRect&& r) noexcept
{
	if (this == &r) {
		return *this;
	}

	x = r.x;
	y = r.y;
	w = r.w;
	h = r.h;
	return *this;
}

bool TextAtlas::AtlasRect::operator==(const AtlasRect& rhs) const
{
	return (x == rhs.x) && (y == rhs.y) && (w == rhs.w) && (h == rhs.h);
}

HRESULT TextAtlas::Initialize(uint32 unWidth, uint32 unHeight, uint32 unPadding /*= 2*/)
{
	HRESULT hr = S_OK;

	m_unWidth = unWidth;
	m_unHeight = unHeight;
	m_unPadding = unPadding;

	m_RenderTarget = TEXTURE->LoadRenderTargetTexture("font_atlas", m_unWidth, m_unHeight, DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_B8G8R8A8_UNORM, D3D12_RESOURCE_STATE_RENDER_TARGET);
	const auto& texResource = m_RenderTarget.GetResource();

	m_srvCPUHandle = texResource->GetSRVHandle();
	m_bHasSRV = true;

	// 1. Create D3D12 render target texture
	const ComPtr<ID3D12Resource>& pd3d12Resource = texResource->GetResourcePtr();
	if (!pd3d12Resource) {
		return E_FAIL;
	}

	D3D11_RESOURCE_FLAGS d3d11ResourceFlags = {};
	d3d11ResourceFlags.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

	// 2. CreateWrappedResource
	hr = RENDER->GetTextRenderer().GetD3D11On12Device()->CreateWrappedResource(
		pd3d12Resource.Get(),
		&d3d11ResourceFlags,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		IID_PPV_ARGS(m_pWrapped11Resource.GetAddressOf())
	);

	if (FAILED(hr)) {
		__debugbreak();
		return hr;
	}

	// 3. Acquire IDXGISurface
	ComPtr<IDXGISurface> pdxgiSurface = nullptr;
	hr = m_pWrapped11Resource->QueryInterface(IID_PPV_ARGS(pdxgiSurface.GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
		return hr;
	}

	// 4. CreateD2D target bitmap
	float fDpiX = RENDER->GetTextRenderer().GetDpiX();
	float fDpiY = RENDER->GetTextRenderer().GetDpiY();

	D2D1_BITMAP_PROPERTIES1 d2dBitmapProps = D2D1::BitmapProperties1(
		D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
		D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
		fDpiX,
		fDpiY
	);

	hr = RENDER->GetTextRenderer().GetD2DDeviceContext()->CreateBitmapFromDxgiSurface(
		pdxgiSurface.Get(),
		&d2dBitmapProps,
		m_pd2dTargetBitmap.GetAddressOf()
	);

	if (FAILED(hr)) {
		__debugbreak();
		return hr;
	}

	Reset();

	return hr;
}

HRESULT TextAtlas::DrawTextToAtlas(const TextLayout& layout, uint32 unAtlasX, uint32 unAtlasY)
{
	if (!layout.pdwTextLayout) {
		__debugbreak();
		return E_INVALIDARG;
	}

	if (!m_pWrapped11Resource || !m_pd2dTargetBitmap) {
		__debugbreak();
		return E_INVALIDARG;
	}

	HRESULT hr = S_OK;

	TextRenderer& textRenderer = RENDER->GetTextRenderer();
	const ComPtr<ID3D11On12Device>& pd3d11On12Device = textRenderer.GetD3D11On12Device();
	const ComPtr<ID3D11DeviceContext>& pd3d11DeviceContext = textRenderer.GetD3D11DeviceContext();
	const ComPtr<ID2D1DeviceContext2>& pd2dDeviceContext = textRenderer.GetD2DDeviceContext();
	const ComPtr<ID2D1SolidColorBrush>& pBrush = textRenderer.GetBrush();

	// 1. AcquireWrappedResource()
	ID3D11Resource* ppd3d11Resources[] = { m_pWrapped11Resource.Get() };
	pd3d11On12Device->AcquireWrappedResources(ppd3d11Resources, 1);

	// 2. Set Target to D2DDeviceContext
	pd2dDeviceContext->SetTarget(m_pd2dTargetBitmap.Get());

	// 3. BeginDraw
	pd2dDeviceContext->BeginDraw();

	// 4. Clear using background color
	// Only clears current text area
	D2D1_RECT_F clearRect = D2D1::RectF(
		static_cast<float>(unAtlasX),
		static_cast<float>(unAtlasY),
		static_cast<float>(unAtlasX) + layout.fWidth,
		static_cast<float>(unAtlasY) + layout.fHeight
	);

	pd2dDeviceContext->PushAxisAlignedClip(clearRect, D2D1_ANTIALIAS_MODE_ALIASED);
	pd2dDeviceContext->Clear(D2D1::ColorF(0.f, 0.f, 0.f, 0.f));	// TODO : Make different clear colors for each Layout
	pd2dDeviceContext->PopAxisAlignedClip();

	// 5. Draw text layout

	pd2dDeviceContext->SetTransform(D2D1::Matrix3x2F::Identity());
	pd2dDeviceContext->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);

	pd2dDeviceContext->DrawTextLayout(
		D2D1::Point2F(
			static_cast<float>(unAtlasX),
			static_cast<float>(unAtlasY)
		),
		layout.pdwTextLayout.Get(),
		pBrush.Get(),
		D2D1_DRAW_TEXT_OPTIONS_NONE
	);


	// 6. EndDraw
	hr = pd2dDeviceContext->EndDraw();

	// 7. SetTarget(nullptr)
	pd2dDeviceContext->SetTarget(nullptr);

	// 8. ReleaseWrappedResource()
	pd3d11On12Device->ReleaseWrappedResources(ppd3d11Resources, 1);

	// 9. Flush D3D11 device context
	pd3d11DeviceContext->Flush();

	if (FAILED(hr)) {
		__debugbreak();
		return hr;
	}

	return hr;
}

HRESULT TextAtlas::Clear()
{
	if (!m_pWrapped11Resource || !m_pd2dTargetBitmap) {
		__debugbreak();
		return E_FAIL;
	}

	HRESULT hr = S_OK;

	const ComPtr<ID3D11On12Device>& pd3d11On12Device = RENDER->GetTextRenderer().GetD3D11On12Device();
	const ComPtr<ID3D11DeviceContext>& pd3d11DeviceContext = RENDER->GetTextRenderer().GetD3D11DeviceContext();
	const ComPtr<ID2D1DeviceContext2>& pd2dDeviceContext = RENDER->GetTextRenderer().GetD2DDeviceContext();
	const ComPtr<ID2D1SolidColorBrush>& pBrush = RENDER->GetTextRenderer().GetBrush();

	ID3D11Resource* ppd3d11Resources[] = { m_pWrapped11Resource.Get() };
	pd3d11On12Device->AcquireWrappedResources(ppd3d11Resources, 1);

	pd2dDeviceContext->SetTarget(m_pd2dTargetBitmap.Get());
	pd2dDeviceContext->BeginDraw();
	pd2dDeviceContext->Clear(D2D1::ColorF(0.f, 0.f, 0.f, 0.f));	// TODO : Make different clear colors for each Layout
	hr = pd2dDeviceContext->EndDraw();
	pd2dDeviceContext->SetTarget(nullptr);

	pd3d11On12Device->ReleaseWrappedResources(ppd3d11Resources, 1);
	pd3d11DeviceContext->Flush();

	if (FAILED(hr)) {
		__debugbreak();
		return hr;
	}

	return hr;
}

bool TextAtlas::AllocateAtlasRect(uint32 unWidth, uint32 unHeight, OUT AtlasRect& outRect)
{
	outRect = {};

	if (unWidth == 0 || unHeight == 0) {
		return false;
	}

	const uint32 unRequiredWidth = unWidth + m_unPadding * 2;		// left + right
	const uint32 unRequiredHeight = unHeight + m_unPadding * 2;		// Top + Bottom

	int32 nBestIndex = -1;
	uint64 un64BestWasteArea = std::numeric_limits<uint64>::max();

	for (int32 i = 0; i < m_FreeRects.size(); ++i) {
		const AtlasRect& freeRect = m_FreeRects[i];
		
		if (unRequiredWidth > freeRect.w || unRequiredHeight > freeRect.h) {
			continue;
		}

		const uint64 un64WasteArea = static_cast<uint64>(freeRect.w) * freeRect.h - static_cast<uint64>(unRequiredWidth) * unRequiredHeight;
		if (un64WasteArea < un64BestWasteArea) {
			un64BestWasteArea = un64WasteArea;
			nBestIndex = i;
		}
	}

	if (nBestIndex < 0) {
		return false;
	}

	AtlasRect chosen = m_FreeRects[nBestIndex];
	m_FreeRects.erase(m_FreeRects.begin() + nBestIndex);

	outRect.x = chosen.x + m_unPadding;
	outRect.y = chosen.y + m_unPadding;
	outRect.w = unWidth;
	outRect.h = unHeight;

	// Split
	const uint32 unRemainRightWidth = chosen.w - unRequiredWidth;
	const uint32 unRemainBottomHeight = chosen.h - unRequiredHeight;

	/*
		           chosen.w
		        +------------+-------------------------+
	   chosen.h	|   chosen   |      New free rect      |
				+------------+-------------------------|
				|                                      |
				|                                      |
				|                                      |
				|            New free rect             |
				|                                      |
				|                                      |
				|                                      |
				|                                      |
				+--------------------------------------+
	*/

	// Right leftovers
	if (unRemainRightWidth > 0) {
		uint32 x = chosen.x + unRequiredWidth;
		uint32 y = chosen.y;
		uint32 w = unRemainRightWidth;
		uint32 h = unRequiredHeight;
		m_FreeRects.emplace_back(x, y, w, h);
	}

	// Bottom leftovers
	if (unRemainBottomHeight > 0) {
		uint32 x = chosen.x;
		uint32 y = chosen.y + unRequiredHeight;
		uint32 w = chosen.w;
		uint32 h = unRemainBottomHeight;
		m_FreeRects.emplace_back(x, y, w, h);
	}

	// Erase small rects
	std::erase_if(m_FreeRects, [](const AtlasRect& r) { return r.w == 0 || r.h == 0; });

	MergeAtlasRect();
	return true;
}

void TextAtlas::FreeAtlasRect(const AtlasRect& rect)
{
	if (rect.w == 0 || rect.h == 0) {
		return;
	}

	// free area
	uint32 x = (rect.x >= m_unPadding) ? (rect.x - m_unPadding) : 0;
	uint32 y = (rect.y >= m_unPadding) ? (rect.y - m_unPadding) : 0;
	uint32 w = rect.w + m_unPadding * 2;
	uint32 h = rect.h + m_unPadding * 2;

	if (x + w > m_unWidth) {
		w = m_unWidth - x;
	}

	if (y + h > m_unHeight) {
		h = m_unHeight - y;
	}

	m_FreeRects.emplace_back(x, y, w, h);
	MergeAtlasRect();
}

void TextAtlas::MergeAtlasRect()
{
	bool bMerged = true;

	while (bMerged) {
		bMerged = false;
		for (uint32 i = 0; i < m_FreeRects.size() && !bMerged; ++i) {
			for (uint32 j =  i + 1; j < m_FreeRects.size() && !bMerged; ++j) {
				AtlasRect& a = m_FreeRects[i];
				AtlasRect& b = m_FreeRects[j];

				// Vertical merge
				if (a.x == b.x && a.w == b.w) {
					if (a.y + a.h == b.y) {
						a.h += b.h;
						m_FreeRects.erase(m_FreeRects.begin() + j);
						bMerged = true;
						break;
					}
					if (b.y + b.h == a.y) {
						a.y = b.y;
						a.h += b.h;
						m_FreeRects.erase(m_FreeRects.begin() + j);
						bMerged = true;
						break;
					}
				}

				// Horizontal merge
				if (a.y == b.y && a.h == b.h) {
					if (a.x + a.w == b.x) {
						a.w += b.w;
						m_FreeRects.erase(m_FreeRects.begin() + j);
						bMerged = true;
						break;
					}
					if (b.x + b.w == a.x) {
						a.x = b.x;
						a.w += b.w;
						m_FreeRects.erase(m_FreeRects.begin() + j);
						bMerged = true;
						break;
					}
				}
			}
		}
	}
}

TextAtlas::RebuildAlert TextAtlas::NeedsAtlasRebuild(uint32 unRequestWidth, uint32 unRequestHeight) const
{
	// Criteria of rebuild
	// 1. Requested size is larger then whole atlas => Alert but rebuild is useless
	// 2. Enough total free area but cannot fit into any free rect => Fragmentation is severe. need to rebuild

	if (unRequestWidth == 0 || unRequestHeight == 0) {
		return RebuildAlert::INVALID_REQUEST;
	}

	const uint32 unPaddedReqW = unRequestWidth + m_unPadding * 2;	// left + right
	const uint32 unPaddedReqH = unRequestHeight + m_unPadding * 2;	// Top + Bottom

	// Requested size is larger then whole atlas => Alert but rebuild is useless
	if (unPaddedReqW > m_unWidth || unPaddedReqH > m_unHeight) {
		return RebuildAlert::NOT_ENOUGH_SPACE;
	}

	uint64 un64TotalFreeArea = 0;
	for (const auto& rect : m_FreeRects) {
		const uint64 un64Area = static_cast<uint64>(rect.w) * rect.h;
		un64TotalFreeArea += un64Area;

		if (unPaddedReqW <= rect.w && unPaddedReqH <= rect.h) {
			return RebuildAlert::OK;
		}
	}

	const uint64 un64ReqArea = static_cast<uint64>(unPaddedReqW) * unPaddedReqH;
	
	// Enough total free area but cannot fit into any free rect => Fragmentation is severe. need to rebuild
	return (un64TotalFreeArea >= un64ReqArea) ? RebuildAlert::SEVERE_FRAGMENTATION : RebuildAlert::NOT_ENOUGH_SPACE;
}

void TextAtlas::Reset()
{
	m_FreeRects.clear();

	if (m_unWidth > 0 && m_unHeight > 0) {
		m_FreeRects.emplace_back(0, 0, m_unWidth, m_unHeight);
	}
}
