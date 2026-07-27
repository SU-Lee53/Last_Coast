#include "pch.h"
#include "TextRenderer.h"

void TextRenderer::Initialize(const ComPtr<ID3D12CommandQueue>& pd3dCommandQueue)
{
	m_pd3dCommandQueue = pd3dCommandQueue;

	// ??
	float fDpi = ::GetDpiForWindow(WinCore::g_hWnd);
	m_fDpiX = fDpi;
	m_fDpiY = fDpi;

	HRESULT hr;
	hr = InitializeD3D11On12(pd3dCommandQueue);
	if (FAILED(hr)) {
		__debugbreak();
		return;
	}

	hr = InitializeD2D1();
	if (FAILED(hr)) {
		__debugbreak();
		return;
	}

	hr = InitializeDirectWrite();
	if (FAILED(hr)) {
		__debugbreak();
		return;
	}

	InitializeLocaleName();

	hr = CreateBundledFontCollection(L"../Resources/Fonts/NotoSansKR-Regular.ttf");
	if (FAILED(hr)) {
		__debugbreak();
		return;
	}

	hr = CreateSharedResources();
	if (FAILED(hr)) {
		__debugbreak();
		return;
	}

	hr = CreateMainAtlas(g_unAtlasWidth, g_unAtlasHeight);
	if (FAILED(hr)) {
		__debugbreak();
		return;
	}

	m_TextTable.Initialize(1000, false);
	m_TextTable.SetCleanUpCallback(
		[this](const std::shared_ptr<CachedText>& pCached) {
			if (!pCached) return;

			if (pCached->bValid) {
				m_TextAtlas->FreeAtlasRect(pCached->Rect);
				pCached->bValid = false;
				pCached->bDirty = false;
				pCached->Rect = {};
			}
		}
	);


	// test
	FontDesc desc;
	desc.wstrFamilyName = L"Noto Sans KR";

	Font::ID fontID = RegisterFont(desc);
}

TextHandle TextRenderer::GetOrCacheText(Font::ID fontID, const std::wstring& wstrText)
{
	HRESULT hr = S_OK;

	TextCacheKey key{ fontID, wstrText };
	auto exist = m_TextTable.GetHandle(key);
	if (exist.IsValid()) {
		const auto& pExist = exist.GetResource();
		if (!pExist) {
			return {};
		}

		if (pExist->bDirty) {
			hr = UpdateCachedText(*pExist);
			if (FAILED(hr)) {
				__debugbreak();
				return {};
			}
		}

		return exist;
	}

	auto handle = CacheText(fontID, wstrText);
	if (!handle.IsValid()) {
		//__debugbreak();
		return {};
	}

	return handle;
}

const TextureRef<RenderTargetTexture>& TextRenderer::GetAtlasTextureRef() const
{
	return m_TextAtlas->GetRenderTarget();
}

Font::ID TextRenderer::RegisterFont(const FontDesc& desc)
{
	if (!m_pdwFactory) return INVALID_ID;

	HRESULT hr;

	Font font{};
	font.desc = desc;

	hr = m_pdwFactory->CreateTextFormat(
		desc.wstrFamilyName.c_str(),
		m_pdwBundledFontCollection.Get(),
		desc.fontWeight,
		desc.fontStyle,
		desc.fontStretch,
		Font::g_fFontSize,
		m_wstrLocaleName.c_str(),
		font.pdwTextFormat.GetAddressOf()
	);

	if (FAILED(hr)) {
		__debugbreak();
		return INVALID_ID;
	}

	hr = font.pdwTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
	if (FAILED(hr)) {
		__debugbreak();
		return INVALID_ID;
	}

	hr = font.pdwTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
	if (FAILED(hr)) {
		__debugbreak();
		return INVALID_ID;
	}

	font.bValid = true;

	//m_Fonts.push_back(font);
	auto [id, res] = m_FontMap.Insert(desc.wstrFamilyName, font);

	// WarmUp
	CacheText(id, L"0123456789");
	CacheText(id, L"ABCDEFGHIJKLMNOPQRSTUVWXYZ");
	CacheText(id, L"abcdefghijklmnopqrstuvwxyz");
	CacheText(id, L"가나다라마바사아자차카타파하");
	CacheText(id, L"The Quick Brown Fox Jumps Over The Lazy Dog");
	CacheText(id, L"다람쥐 헌 쳇바퀴에 타고파");

	return id;
}

HRESULT TextRenderer::CreateBundledFontCollection(const std::wstring& wstrFontPath)
{
	if (!m_pdwFactory) {
		return E_FAIL;
	}

	HRESULT hr{};

	const std::filesystem::path fontPath = std::filesystem::absolute(wstrFontPath);
	
	ComPtr<IDWriteFontFile> pFontFile = nullptr;
	hr = m_pdwFactory->CreateFontFileReference(
		fontPath.c_str(),
		nullptr,
		pFontFile.GetAddressOf()
	);
	if (FAILED(hr)) {
		__debugbreak();
		return hr;
	}

	ComPtr<IDWriteFontSetBuilder1> pFontSetBuilder = nullptr;
	hr = m_pdwFactory->CreateFontSetBuilder(pFontSetBuilder.GetAddressOf());
	if (FAILED(hr)) {
		__debugbreak();
		return hr;
	}

	hr = pFontSetBuilder->AddFontFile(pFontFile.Get());
	if (FAILED(hr)) {
		__debugbreak();
		return hr;
	}

	ComPtr<IDWriteFontSet> pFontSet = nullptr;
	hr = pFontSetBuilder->CreateFontSet(pFontSet.GetAddressOf());
	if (FAILED(hr)) {
		__debugbreak();
		return hr;
	}

	hr = m_pdwFactory->CreateFontCollectionFromFontSet(pFontSet.Get(), m_pdwBundledFontCollection.GetAddressOf());
	if (FAILED(hr)) {
		__debugbreak();
		return hr;
	}

	return S_OK;
}

void TextRenderer::DestroyFont(const Font::ID& id)
{
	if (id == INVALID_ID) return;
	if (id >= m_FontMap.Size()) return;

	m_FontMap[id].pdwTextFormat.Reset();
	m_FontMap[id].bValid = false;
}

void TextRenderer::DestroyFont(const std::wstring& wstrName)
{
	auto pFont = m_FontMap.Find(wstrName);
	if (!pFont) return;

	pFont->pdwTextFormat.Reset();
	pFont->bValid = false;
}

const Font* TextRenderer::GetFont(Font::ID id)
{
	if (id == INVALID_ID) return nullptr;
	if (id >= m_FontMap.Size()) return nullptr;
	if (m_FontMap[id].bValid == false) return nullptr;

	return &m_FontMap[id];
}

const Font* TextRenderer::GetFont(const std::wstring& wstrName)
{
	return m_FontMap.Find(wstrName);
}

const Font::ID TextRenderer::GetFontID(const std::wstring& wstrName)
{
	return m_FontMap.GetIndex(wstrName);
}

HRESULT TextRenderer::CreateTextLayout(Font::ID fontID, const std::wstring& wstrText, float fMaxWidth, float fMaxHeight, OUT TextLayout& outLayout)
{
	HRESULT hr = S_OK;

	const Font* pFont = GetFont(fontID);
	if (!pFont) {
		__debugbreak(); 
		return E_INVALIDARG;
	}

	if (!pFont->pdwTextFormat) {
		__debugbreak();
		return E_FAIL;
	}
	
	outLayout = {};
	hr = m_pdwFactory->CreateTextLayout(
		wstrText.c_str(),
		static_cast<UINT32>(wstrText.length()),
		pFont->pdwTextFormat.Get(),
		fMaxWidth,
		fMaxHeight,
		outLayout.pdwTextLayout.GetAddressOf());

	if (FAILED(hr)) {
		__debugbreak();
		return hr;
	}

	DWRITE_TEXT_METRICS dwMetrics{};
	hr = outLayout.pdwTextLayout->GetMetrics(&dwMetrics);
	if (FAILED(hr)) {
		__debugbreak();
		return hr;
	}

	outLayout.fWidth = std::ceil(dwMetrics.widthIncludingTrailingWhitespace);
	outLayout.fHeight = std::ceil(dwMetrics.height);

	return S_OK;
}

TextHandle TextRenderer::CacheText(Font::ID fontID, const wstring& wstrText)
{
	HRESULT hr = S_OK;
	TextCacheKey key{ fontID, wstrText };
	auto exist = m_TextTable.GetHandle(key);

	if (exist.IsValid()) {
		const auto& pExist = exist.GetResource();
		if (!pExist) {
			return {};
		}

		if (pExist->bDirty == true) {
			hr = UpdateCachedText(*pExist);
			if (FAILED(hr)) {
				__debugbreak();
				return {};
			}
		}
		return {};
	}

	TextLayout layout{};
	hr = CreateTextLayout(fontID, wstrText, 4096.f, 4096.f, layout);
	if (FAILED(hr)) {
		__debugbreak();
		return {};
	}

	TextAtlas::AtlasRect rect{};
	bool bAllocateResult = m_TextAtlas->AllocateAtlasRect(static_cast<uint32>(layout.fWidth), static_cast<uint32>(layout.fHeight), rect);
	if (!bAllocateResult) {
		TextAtlas::RebuildAlert eAlert = m_TextAtlas->NeedsAtlasRebuild(static_cast<uint32>(layout.fWidth), static_cast<uint32>(layout.fHeight));

		switch (eAlert)
		{
		case TextAtlas::RebuildAlert::NOT_ENOUGH_SPACE:
		{
			return {};
		}
		case TextAtlas::RebuildAlert::SEVERE_FRAGMENTATION:
		{
			hr = RebuildAtlas();
			if (FAILED(hr)) {
				__debugbreak();
				return {};
			}
			
			return CacheText(fontID, wstrText);	// Retry after rebuild

		}
		case TextAtlas::RebuildAlert::INVALID_REQUEST:
		{
			return {};
		}
		default:
			break;
		}

		return {};
	}

	hr = m_TextAtlas->DrawTextToAtlas(layout, rect.x, rect.y);

	if (FAILED(hr)) {
		m_TextAtlas->FreeAtlasRect(rect);
		return {};
	}

	std::shared_ptr<CachedText> pCached = std::make_shared<CachedText>();
	pCached->fontID = fontID;
	pCached->wstrText = wstrText;
	pCached->Rect = rect;
	pCached->bValid = true;
	pCached->bDirty = false;

	return m_TextTable.Register(key, pCached);;
}

HRESULT TextRenderer::UpdateCachedText(OUT CachedText& cachedText)
{
	if (!cachedText.bValid) {
		return E_FAIL;
	}

	HRESULT hr;

	TextLayout layout{};
	hr = CreateTextLayout(
		cachedText.fontID,
		cachedText.wstrText,
		4096.f,
		4096.f,
		layout
	);

	if (FAILED(hr)) {
		__debugbreak();
		return hr;
	}

	const uint32 unNewWidth = static_cast<uint32>(layout.fWidth);
	const uint32 unNewHeight = static_cast<uint32>(layout.fHeight);
	const bool bFitsInPlace = (unNewWidth <= cachedText.Rect.w) && (unNewHeight <= cachedText.Rect.h);

	// 1. Recycle rect if fits
	if (bFitsInPlace) {
		hr = m_TextAtlas->DrawTextToAtlas(layout, cachedText.Rect.x, cachedText.Rect.y);

		if (FAILED(hr)) {
			__debugbreak();
			return hr;
		}

		cachedText.bDirty = false;
		return hr;
	}

	// 2. Try reallocate text if not fits
	TextAtlas::AtlasRect newRect{};
	bool bAllocResult = m_TextAtlas->AllocateAtlasRect(unNewWidth, unNewHeight, newRect);
	
	if (!bAllocResult) {
		TextAtlas::RebuildAlert eAlert = m_TextAtlas->NeedsAtlasRebuild(unNewWidth, unNewHeight);
		
		// Rebuild atlas if fragmentation is severe
		if (eAlert == TextAtlas::RebuildAlert::SEVERE_FRAGMENTATION) {
			hr = RebuildAtlas();
			if (FAILED(hr)) {
				__debugbreak();
				cachedText.bDirty = true;
				return hr;
			}

			TextCacheKey key{ cachedText.fontID, cachedText.wstrText };
			auto handle = m_TextTable.GetHandle(key);
			if (!handle.IsValid() || !handle.GetResource()) {
				return E_FAIL;
			}

			auto& pUpdated = handle.GetResource();
			pUpdated->bDirty = false;

			return S_OK;
		}
		
		// fail if rebuild cannot solve problem
		cachedText.bDirty = true;
		return E_FAIL;
	}

	// 3. Reallocation successful, draw new text
	hr = m_TextAtlas->DrawTextToAtlas(layout, newRect.x, newRect.y);

	if (FAILED(hr)) {
		__debugbreak();

		// Free new rect
		m_TextAtlas->FreeAtlasRect(newRect);
		cachedText.bDirty = true;
		return hr;
	}

	// 4. Free old rect 
	m_TextAtlas->FreeAtlasRect(cachedText.Rect);

	// 5. Set cachedText to new rect
	cachedText.Rect = newRect;
	cachedText.bDirty = false;

	return hr;
}

HRESULT TextRenderer::CreateMainAtlas(uint32 unWidth, uint32 unHeight)
{
	if (unWidth == 0 || unHeight == 0) {
		return E_INVALIDARG;
	}

	m_TextAtlas = std::make_unique<TextAtlas>();
	return m_TextAtlas->Initialize(unWidth, unHeight, 2);
}

HRESULT TextRenderer::RebuildAtlas()
{
	HRESULT hr = S_OK;

	if (!m_TextAtlas->GetRenderTarget().IsValid()) {
		return E_FAIL;
	}

	m_TextAtlas->Reset();
	hr = m_TextAtlas->Clear();
	if (FAILED(hr)) {
		__debugbreak();
		return hr;
	}

	for (auto& entry : m_TextTable.GetEntries()) {
		if (!entry.bAlive) {
			continue;
		}
		
		auto& cached = *entry.pResource;
		if (!cached.bValid) {
			continue;
		}

		TextLayout layout{};
		hr = CreateTextLayout(cached.fontID, cached.wstrText, 4096.f, 4096.f, layout);
		if (FAILED(hr)) {
			__debugbreak();
			return hr;
		}

		TextAtlas::AtlasRect rect{};
		bool bAllocResult = m_TextAtlas->AllocateAtlasRect(
			static_cast<uint32>(layout.fWidth),
			static_cast<uint32>(layout.fHeight),
			rect
		);

		if (!bAllocResult) {
			__debugbreak();
			return E_FAIL;
		}

		hr = m_TextAtlas->DrawTextToAtlas(layout, rect.x, rect.y);
		if (FAILED(hr)) {
			__debugbreak();
			return hr;
		}

		cached.Rect = rect;
		cached.bDirty = false;
	}

	return hr;
}

HRESULT TextRenderer::InitializeD3D11On12(const ComPtr<ID3D12CommandQueue>& pd3dCommandQueue)
{
	HRESULT hr = S_OK;
	UINT d3d11Flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

	if (D3DCore::g_bEnableDebugLayer) {
		d3d11Flags |= D3D11_CREATE_DEVICE_DEBUG;
	}

	IUnknown* ppCommandQueue[] = { pd3dCommandQueue.Get() };

	// Create D3D11On12
	hr = D3D11On12CreateDevice(
		DEVICE.Get(),
		d3d11Flags,
		nullptr,
		0,
		ppCommandQueue,
		1,
		0,
		m_pd3d11Device.GetAddressOf(),
		m_pd3d11DeviceContext.GetAddressOf(),
		nullptr
	);

	if (FAILED(hr)) {
		__debugbreak();
		return hr;
	}

	hr = m_pd3d11Device->QueryInterface(IID_PPV_ARGS(m_pd3d11On12Device.GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
		return hr;
	}

	return hr;
}

HRESULT TextRenderer::InitializeD2D1()
{
	HRESULT hr = S_OK;

	// Create D2D1 / DWrite components
	D2D1_FACTORY_OPTIONS d2dFactoryOptions = {};
	if (D3DCore::g_bEnableDebugLayer) {
		d2dFactoryOptions.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
	}

	hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory3), &d2dFactoryOptions, (void**)m_pd2dFactory.GetAddressOf());
	if (FAILED(hr)) {
		__debugbreak();
		return hr;
	}

	ComPtr<IDXGIDevice> pdxgiDevice = nullptr;
	hr = m_pd3d11Device->QueryInterface(IID_PPV_ARGS(pdxgiDevice.GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
		return hr;
	}

	hr = m_pd2dFactory->CreateDevice(pdxgiDevice.Get(), m_pd2dDevice.GetAddressOf());
	if (FAILED(hr)) {
		__debugbreak();
		return hr;
	}

	D2D1_DEVICE_CONTEXT_OPTIONS d2dDeviceOptions = D2D1_DEVICE_CONTEXT_OPTIONS_NONE;
	hr = m_pd2dDevice->CreateDeviceContext(d2dDeviceOptions, m_pd2dDeviceContext.GetAddressOf());
	if (FAILED(hr)) {
		__debugbreak();
		return hr;
	}

	return hr;
}

HRESULT TextRenderer::InitializeDirectWrite()
{
	HRESULT hr = S_OK;
	hr = ::DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory5), (IUnknown**)(m_pdwFactory.GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
		return hr;
	}

	return hr;
}

// Create D2D1 <-> D3D shared resources
HRESULT TextRenderer::CreateSharedResources()
{
	HRESULT hr = S_OK;

	// Set DPI to D2D1DeviceContext
	m_pd2dDeviceContext->SetDpi(m_fDpiX, m_fDpiY);

	// Create default white brush
	hr = m_pd2dDeviceContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White, 1.0f), m_pBrush.GetAddressOf());
	if (FAILED(hr)) {
		__debugbreak();
		return hr;
	}

	return hr;
}

void TextRenderer::InitializeLocaleName()
{
	WCHAR pwchLocaleName[LOCALE_NAME_MAX_LENGTH] = {};
	if (::GetUserDefaultLocaleName(pwchLocaleName, LOCALE_NAME_MAX_LENGTH) > 0) {
		m_wstrLocaleName = pwchLocaleName;
		return;
	}

	m_wstrLocaleName = L"en-US";
}
