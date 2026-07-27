#include "pch.h"
#include "TextureManager.h"

uint32 TextureManager::g_unRTVFromCoreCount = 0;
uint32 TextureManager::g_unDSVFromCoreCount = 0;

void TextureManager::Initialize(ComPtr<ID3D12Device> pd3dDevice)
{
	m_pd3dDevice = pd3dDevice;

	CreateCommandList();
	CreateFence();

	//// Create DescriptorHeaps + Table
	m_SRVTextureTable.Initialize(pd3dDevice, MAX_TEXTURE_COUNT, true, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
	m_UAVTextureTable.Initialize(pd3dDevice, 50, true, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
	m_RTVTextureTable.Initialize(pd3dDevice, 50, true, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
	m_DSVTextureTable.Initialize(pd3dDevice, 50, true, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

	m_CommandListPool.Initialize(pd3dDevice);

	LoadGameTextures();
}

void TextureManager::LoadGameTextures()
{
	// Font
	//LoadTexture("font");

	m_DebugAlbedo = LoadTexture("DefaultMaterial_BaseColor_0");
	m_DebugNormal = LoadTexture("DefaultMaterial_Normal_0");

}

TextureRef<Texture> TextureManager::LoadTexture(const std::string& strTextureName, bool bCheckTransparent)
{
	if (strTextureName == "None") return {};

	// 1. return immediatly if texture already loaded
	{
		std::lock_guard tableLock{ m_mtxTextureLoad };
		TextureHandle findHandle = m_SRVTextureTable.GetHandle(strTextureName);
		if (findHandle.IsValid()) {
			return { findHandle };
		}
	}


	auto pLoadMutex = GetTextureLoadMutex(strTextureName);
	std::lock_guard keyLock{ *pLoadMutex };

	// 2. Check again. (other thread can load same texture while wait for key lock)
	{
		std::lock_guard tableLock{ m_mtxTextureLoad };
		TextureHandle findHandle = m_SRVTextureTable.GetHandle(strTextureName);
		if (findHandle.IsValid()) {
			return { findHandle };
		}
	}

	// 3. Do heavy (file loading, GPU Copy) works without global lock
	std::shared_ptr<Texture> pTexture = std::make_shared<Texture>();
	bool bResult = pTexture->CreateTextureFromFile(::StringToWString(strTextureName), bCheckTransparent);
	if (!bResult) {
		return {};
	}

	TextureTable::ResourceDesc srvDesc;
	srvDesc.eType = TextureTable::ResourceDesc::TYPE::SRV;
	srvDesc.eDimension = TextureTable::ResourceDesc::DIMENSION::TEXTURE2D;

	TextureHandle SRVHandle;

	// 4. Uses global table lock for table registeration
	{
		std::lock_guard tableLock{ m_mtxTextureLoad };
		SRVHandle = m_SRVTextureTable.Register(strTextureName, pTexture, &srvDesc);

		if (!SRVHandle.IsValid()) {
			OutputDebugStringA(std::format("Failed to load texture SRV : {}", strTextureName).c_str());
			return {};
		}

		pTexture->m_d3dSRVDesc = srvDesc.srv;
		pTexture->m_un64RuntimeSRVID = SRVHandle.GetID();
		pTexture->m_d3dSRVHandle = m_SRVTextureTable.GetCPUHandleByHandle(SRVHandle);
	}

	return { SRVHandle };
}

TextureRef<Texture> TextureManager::LoadTextureFromRaw(const std::string& strTextureName, uint32 unWidth, uint32 unHeight)
{
	{
		std::lock_guard tableLock{ m_mtxTextureLoad };
		TextureHandle findHandle = m_SRVTextureTable.GetHandle(strTextureName);
		if (findHandle.IsValid()) {
			return { findHandle };
		}
	}

	auto pLoadMutex = GetTextureLoadMutex(strTextureName);
	std::lock_guard keyLock{ *pLoadMutex };

	{
		std::lock_guard tableLock{ m_mtxTextureLoad };
		TextureHandle findHandle = m_SRVTextureTable.GetHandle(strTextureName);
		if (findHandle.IsValid()) {
			return { findHandle };
		}
	}

	std::shared_ptr<Texture> pTexture = std::make_shared<Texture>();
	bool bResult = pTexture->CreateTextureFromRawFile(::StringToWString(strTextureName), unWidth, unHeight);
	if (!bResult) {
		return {};
	}

	TextureTable::ResourceDesc srvDesc;
	srvDesc.eType = TextureTable::ResourceDesc::TYPE::SRV;
	srvDesc.eDimension = TextureTable::ResourceDesc::DIMENSION::TEXTURE2D;

	TextureHandle SRVHandle;
	{
		std::lock_guard tableLock{ m_mtxTextureLoad };
		SRVHandle = m_SRVTextureTable.Register(
			strTextureName,
			pTexture,
			&srvDesc);

		if (SRVHandle.IsValid()) {
			OutputDebugStringA(std::format("Failed to load texture SRV : {}", strTextureName).c_str());
			return {};
		}

		pTexture->m_d3dSRVDesc = srvDesc.srv;
		pTexture->m_un64RuntimeSRVID = SRVHandle.GetID();
		pTexture->m_d3dSRVHandle = m_SRVTextureTable.GetCPUHandleByHandle(SRVHandle);
	}

	return { SRVHandle };
}

TextureRef<Texture> TextureManager::LoadTextureArray(const std::string& strTextureName, const std::wstring& wstrTexturePath)
{
	{
		std::lock_guard tableLock{ m_mtxTextureLoad };
		TextureHandle findHandle = m_SRVTextureTable.GetHandle(strTextureName);
		if (findHandle.IsValid()) {
			return { findHandle };
		}
	}

	auto pLoadMutex = GetTextureLoadMutex(strTextureName);
	std::lock_guard keyLock{ *pLoadMutex };

	{
		std::lock_guard tableLock{ m_mtxTextureLoad };
		TextureHandle findHandle = m_SRVTextureTable.GetHandle(strTextureName);
		if (findHandle.IsValid()) {
			return { findHandle };
		}
	}

	std::shared_ptr<Texture> pTexture = std::make_shared<Texture>();
	bool bResult = pTexture->CreateTextureArrayFromFile(::StringToWString(strTextureName));
	if (!bResult) {
		return {};
	}

	TextureTable::ResourceDesc srvDesc;
	srvDesc.eType = TextureTable::ResourceDesc::TYPE::SRV;
	srvDesc.eDimension = TextureTable::ResourceDesc::DIMENSION::TEXTURE2DARRAY;

	TextureHandle SRVHandle;
	{
		std::lock_guard tableLock{ m_mtxTextureLoad };
		SRVHandle = m_SRVTextureTable.Register(
			strTextureName,
			pTexture,
			&srvDesc);

		if (!SRVHandle.IsValid()) {
			OutputDebugStringA(std::format("Failed to load texture SRV : {}", strTextureName).c_str());
			return {};
		}

		pTexture->m_d3dSRVDesc = srvDesc.srv;
		pTexture->m_un64RuntimeSRVID = SRVHandle.GetID();
		pTexture->m_d3dSRVHandle = m_SRVTextureTable.GetCPUHandleByHandle(SRVHandle);
	}

	return { SRVHandle };
}

TextureRef<Texture> TextureManager::LoadTextureFromRawData(const std::string& strTextureName, const std::vector<Vector4>& data, uint32 unWidth, uint32 unHeight, DXGI_FORMAT dxgiSRVFormat)
{
	{
		std::lock_guard tableLock{ m_mtxTextureLoad };
		TextureHandle findHandle = m_SRVTextureTable.GetHandle(strTextureName);
		if (findHandle.IsValid()) {
			return { findHandle };
		}
	}

	auto pLoadMutex = GetTextureLoadMutex(strTextureName);
	std::lock_guard keyLock{ *pLoadMutex };

	{
		std::lock_guard tableLock{ m_mtxTextureLoad };
		TextureHandle findHandle = m_SRVTextureTable.GetHandle(strTextureName);
		if (findHandle.IsValid()) {
			return { findHandle };
		}
	}

	std::shared_ptr<Texture> pTexture = std::make_shared<Texture>();
	bool bResult = pTexture->CreateTextureFromRawData(::StringToWString(strTextureName),data, unWidth, unHeight, dxgiSRVFormat);
	if (!bResult) {
		return {};
	}

	TextureTable::ResourceDesc srvDesc;
	srvDesc.eType = TextureTable::ResourceDesc::TYPE::SRV;
	srvDesc.eDimension = TextureTable::ResourceDesc::DIMENSION::TEXTURE2D;

	TextureHandle SRVHandle;
	{
		std::lock_guard tableLock{ m_mtxTextureLoad };
		SRVHandle = m_SRVTextureTable.Register(
			strTextureName,
			pTexture,
			&srvDesc);

		if (!SRVHandle.IsValid()) {
			OutputDebugStringA(std::format("Failed to load texture SRV : {}", strTextureName).c_str());
			return {};
		}

		pTexture->m_d3dSRVDesc = srvDesc.srv;
		pTexture->m_un64RuntimeSRVID = SRVHandle.GetID();
		pTexture->m_d3dSRVHandle = m_SRVTextureTable.GetCPUHandleByHandle(SRVHandle);
	}

	return { SRVHandle };
}

TextureRef<Texture> TextureManager::LoadTextureFromHeightData(const std::string& strTextureName, const std::vector<uint16>& data, uint32 unWidth, uint32 unHeight)
{
	{
		std::lock_guard tableLock{ m_mtxTextureLoad };
		TextureHandle findHandle = m_SRVTextureTable.GetHandle(strTextureName);
		if (findHandle.IsValid()) {
			return { findHandle };
		}
	}

	auto pLoadMutex = GetTextureLoadMutex(strTextureName);
	std::lock_guard keyLock{ *pLoadMutex };

	{
		std::lock_guard tableLock{ m_mtxTextureLoad };
		TextureHandle findHandle = m_SRVTextureTable.GetHandle(strTextureName);
		if (findHandle.IsValid()) {
			return { findHandle };
		}
	}

	std::shared_ptr<Texture> pTexture = std::make_shared<Texture>();
	bool bResult = pTexture->CreateTextureFromHeightData(::StringToWString(strTextureName), data, unWidth, unHeight);
	if (!bResult) {
		return {};
	}

	TextureTable::ResourceDesc srvDesc;
	srvDesc.eType = TextureTable::ResourceDesc::TYPE::SRV;
	srvDesc.eDimension = TextureTable::ResourceDesc::DIMENSION::TEXTURE2D;

	TextureHandle SRVHandle;
	{
		std::lock_guard tableLock{ m_mtxTextureLoad };
		SRVHandle = m_SRVTextureTable.Register(
			strTextureName,
			pTexture,
			&srvDesc);

		if (!SRVHandle.IsValid()) {
			OutputDebugStringA(std::format("Failed to load texture SRV : {}", strTextureName).c_str());
			return {};
		}

		pTexture->m_d3dSRVDesc = srvDesc.srv;
		pTexture->m_un64RuntimeSRVID = SRVHandle.GetID();
		pTexture->m_d3dSRVHandle = m_SRVTextureTable.GetCPUHandleByHandle(SRVHandle);
	}

	return { SRVHandle };
}

TextureRef<RenderTargetTexture> TextureManager::LoadRenderTargetTexture(const std::string& strTextureName, uint32 unWidth, uint32 unHeight, DXGI_FORMAT dxgiSRVFormat, DXGI_FORMAT dxgiRTVFormat, D3D12_RESOURCE_STATES d3dInitialState, float* pfClearValue)
{
	{
		std::lock_guard tableLock{ m_mtxTextureLoad };
		TextureHandle SRVFindHandle = m_SRVTextureTable.GetHandle(strTextureName);
		if (SRVFindHandle.IsValid()) {
			TextureHandle RTVFindHandle = m_RTVTextureTable.GetHandle(strTextureName);
			return { SRVFindHandle, RTVFindHandle };
		}
	}

	auto pLoadMutex = GetTextureLoadMutex(strTextureName);
	std::lock_guard keyLock{ *pLoadMutex };

	{
		std::lock_guard tableLock{ m_mtxTextureLoad };
		TextureHandle SRVFindHandle = m_SRVTextureTable.GetHandle(strTextureName);
		if (SRVFindHandle.IsValid()) {
			TextureHandle RTVFindHandle = m_RTVTextureTable.GetHandle(strTextureName);
			return { SRVFindHandle, RTVFindHandle };
		}
	}

	std::shared_ptr<RenderTargetTexture> pTexture = std::make_shared<RenderTargetTexture>();
	bool bResult = pTexture->Initialize(unWidth, unHeight, dxgiSRVFormat, dxgiRTVFormat, d3dInitialState, pfClearValue);
	if (!bResult) {
		return { {}, {} };
	}

	TextureHandle SRVHandle;
	TextureHandle RTVHandle;
	{
		std::lock_guard tableLock{ m_mtxTextureLoad };

		// SRV
		TextureTable::ResourceDesc srvDesc;
		srvDesc.eType = TextureTable::ResourceDesc::TYPE::SRV;
		srvDesc.eDimension = TextureTable::ResourceDesc::DIMENSION::TEXTURE2D;
		SRVHandle = m_SRVTextureTable.Register(
			strTextureName,
			pTexture,
			&srvDesc,
			&dxgiSRVFormat,
			sizeof(DXGI_FORMAT));

		if (!SRVHandle.IsValid()) {
			OutputDebugStringA(std::format("Failed to load texture SRV : {}", strTextureName).c_str());
			return { {}, {} };
		}

		pTexture->m_d3dSRVDesc = srvDesc.srv;
		pTexture->m_un64RuntimeSRVID = SRVHandle.GetID();
		pTexture->m_d3dSRVHandle = m_SRVTextureTable.GetCPUHandleByHandle(SRVHandle);

		// RTV
		TextureTable::ResourceDesc rtvDesc;
		rtvDesc.eType = TextureTable::ResourceDesc::TYPE::RTV;
		rtvDesc.eDimension = TextureTable::ResourceDesc::DIMENSION::TEXTURE2D;
		RTVHandle = m_RTVTextureTable.Register(
			strTextureName, 
			pTexture, 
			&rtvDesc,
			&dxgiRTVFormat, 
			sizeof(DXGI_FORMAT));

		if (!RTVHandle.IsValid()) {
			OutputDebugStringA(std::format("Failed to load texture RTV : {}", strTextureName).c_str());
			return { {}, {} };
		}

		pTexture->m_d3dRTVDesc = rtvDesc.rtv;
		pTexture->m_un64RuntimeRTVID = RTVHandle.GetID();
		pTexture->m_d3dRTVHandle = m_RTVTextureTable.GetCPUHandleByHandle(RTVHandle);
	}

	return { SRVHandle, RTVHandle };
}

TextureRef<RenderTargetTexture> TextureManager::LoadRenderTargetTexture(ComPtr<ID3D12Resource> pd3dRTVResourceFromSwapChain, DXGI_FORMAT dxgiSRVFormat, DXGI_FORMAT dxgiRTVFormat)
{
	std::string strTextureName;
	{
		std::lock_guard tableLock{ m_mtxTextureLoad };
		strTextureName = "RTV_" + std::to_string(g_unRTVFromCoreCount++);
	}

	std::shared_ptr<RenderTargetTexture> pTexture = std::make_shared<RenderTargetTexture>();
	bool bResult = pTexture->Initialize(pd3dRTVResourceFromSwapChain);

	TextureHandle SRVHandle;
	TextureHandle RTVHandle;
	{
		std::lock_guard tableLock{ m_mtxTextureLoad };

		// SRV
		TextureTable::ResourceDesc srvDesc;
		srvDesc.eType = TextureTable::ResourceDesc::TYPE::SRV;
		srvDesc.eDimension = TextureTable::ResourceDesc::DIMENSION::TEXTURE2D;
		SRVHandle = m_SRVTextureTable.Register(
			strTextureName,
			pTexture,
			&srvDesc,
			&dxgiSRVFormat,
			sizeof(DXGI_FORMAT));

		if (!SRVHandle.IsValid()) {
			OutputDebugStringA(std::format("Failed to load texture SRV : {}", strTextureName).c_str());
			return { {}, {} };
		}

		pTexture->m_d3dSRVDesc = srvDesc.srv;
		pTexture->m_un64RuntimeSRVID = SRVHandle.GetID();
		pTexture->m_d3dSRVHandle = m_SRVTextureTable.GetCPUHandleByHandle(SRVHandle);

		// RTV
		TextureTable::ResourceDesc rtvDesc;
		rtvDesc.eType = TextureTable::ResourceDesc::TYPE::RTV;
		rtvDesc.eDimension = TextureTable::ResourceDesc::DIMENSION::TEXTURE2D;
		RTVHandle = m_RTVTextureTable.Register(
			strTextureName,
			pTexture,
			&rtvDesc,
			&dxgiRTVFormat,
			sizeof(DXGI_FORMAT));

		if (!RTVHandle.IsValid()) {
			OutputDebugStringA(std::format("Failed to load texture RTV : {}", strTextureName).c_str());
			return { {}, {} };
		}

		pTexture->m_d3dRTVDesc = rtvDesc.rtv;
		pTexture->m_un64RuntimeRTVID = RTVHandle.GetID();
		pTexture->m_d3dRTVHandle = m_RTVTextureTable.GetCPUHandleByHandle(RTVHandle);
	}

	return { SRVHandle, RTVHandle };
}

TextureRef<DepthStencilTexture> TextureManager::LoadDepthStencilTexture(const std::string& strTextureName, uint32 unWidth, uint32 unHeight, DXGI_FORMAT dxgiSRVFormat, DXGI_FORMAT dxgiDSVFormat)
{
	{
		std::lock_guard tableLock{ m_mtxTextureLoad };
		TextureHandle SRVFindHandle = m_SRVTextureTable.GetHandle(strTextureName);
		if (SRVFindHandle.IsValid()) {
			TextureHandle DSVFindHandle = m_DSVTextureTable.GetHandle(strTextureName);
			return { SRVFindHandle, DSVFindHandle };
		}
	}

	auto pLoadMutex = GetTextureLoadMutex(strTextureName);
	std::lock_guard keyLock{ *pLoadMutex };

	{
		std::lock_guard tableLock{ m_mtxTextureLoad };
		TextureHandle SRVFindHandle = m_SRVTextureTable.GetHandle(strTextureName);
		if (SRVFindHandle.IsValid()) {
			TextureHandle DSVFindHandle = m_DSVTextureTable.GetHandle(strTextureName);
			return { SRVFindHandle, DSVFindHandle };
		}
	}

	std::shared_ptr<DepthStencilTexture> pTexture = std::make_shared<DepthStencilTexture>();
	bool bResult = pTexture->Initialize(unWidth, unHeight, D3DCore::g_bMsaa4xEnable, dxgiSRVFormat, dxgiDSVFormat);;
	if (!bResult) {
		return { {}, {} };
	}

	TextureHandle SRVHandle;
	TextureHandle DSVHandle;
	{
		std::lock_guard tableLock{ m_mtxTextureLoad };

		// SRV
		TextureTable::ResourceDesc srvDesc;
		srvDesc.eType = TextureTable::ResourceDesc::TYPE::SRV;
		srvDesc.eDimension = TextureTable::ResourceDesc::DIMENSION::TEXTURE2D;
		SRVHandle = m_SRVTextureTable.Register(
			strTextureName,
			pTexture,
			&srvDesc,
			&dxgiSRVFormat,
			sizeof(DXGI_FORMAT));

		if (!SRVHandle.IsValid()) {
			OutputDebugStringA(std::format("Failed to load texture SRV : {}", strTextureName).c_str());
			return { {}, {} };
		}

		pTexture->m_d3dSRVDesc = srvDesc.srv;
		pTexture->m_un64RuntimeSRVID = SRVHandle.GetID();
		pTexture->m_d3dSRVHandle = m_SRVTextureTable.GetCPUHandleByHandle(SRVHandle);
		
		// DSV
		TextureTable::ResourceDesc dsvDesc;
		dsvDesc.eType = TextureTable::ResourceDesc::TYPE::DSV;
		dsvDesc.eDimension = TextureTable::ResourceDesc::DIMENSION::TEXTURE2D;
		DSVHandle = m_DSVTextureTable.Register(
			strTextureName,
			pTexture,
			&dsvDesc,
			&dxgiDSVFormat, sizeof(DXGI_FORMAT));

		if (!DSVHandle.IsValid()) {
			OutputDebugStringA(std::format("Failed to load texture RTV : {}", strTextureName).c_str());
			return { {}, {} };
		}

		pTexture->m_d3dDSVDesc = dsvDesc.dsv;
		pTexture->m_un64RuntimeDSVID = DSVHandle.GetID();
		pTexture->m_d3dDSVHandle = m_DSVTextureTable.GetCPUHandleByHandle(DSVHandle);
	}

	return { SRVHandle, DSVHandle };
}

TextureRef<UnorderedAccessTexture> TextureManager::LoadUnorderedAccessTexture(const std::string& strTextureName, uint32 unArraySize, uint32 unWidth, uint32 unHeight, uint32 unDepth, DXGI_FORMAT dxgiSRVUAVFormat)
{
	{
		std::lock_guard tableLock{ m_mtxTextureLoad };
		TextureHandle SRVFindHandle = m_SRVTextureTable.GetHandle(strTextureName);
		if (SRVFindHandle.IsValid()) {
			TextureHandle UAVFindHandle = m_UAVTextureTable.GetHandle(strTextureName);
			return { SRVFindHandle, UAVFindHandle };
		}
	}

	auto pLoadMutex = GetTextureLoadMutex(strTextureName);
	std::lock_guard keyLock{ *pLoadMutex };

	{
		std::lock_guard tableLock{ m_mtxTextureLoad };
		TextureHandle SRVFindHandle = m_SRVTextureTable.GetHandle(strTextureName);
		if (SRVFindHandle.IsValid()) {
			TextureHandle UAVFindHandle = m_UAVTextureTable.GetHandle(strTextureName);
			return { SRVFindHandle, UAVFindHandle };
		}
	}

	std::shared_ptr<UnorderedAccessTexture> pTexture = std::make_shared<UnorderedAccessTexture>();
	bool bResult = (unArraySize == 1) ? (unDepth == 0) ? pTexture->Initialize(unWidth, unHeight, dxgiSRVUAVFormat)
		                                               : pTexture->Initialize3D(unWidth, unHeight, unDepth, dxgiSRVUAVFormat)
		                              : pTexture->InitializeArray(unArraySize, unWidth, unHeight, dxgiSRVUAVFormat);
	if (!bResult) {
		return { {}, {} };
	}

	TextureHandle SRVHandle;
	TextureHandle UAVHandle;
	{
		std::lock_guard tableLock{ m_mtxTextureLoad };

		// SRV
		TextureTable::ResourceDesc srvDesc;
		srvDesc.eType = TextureTable::ResourceDesc::TYPE::SRV;
		if (unArraySize == 1) {
			srvDesc.eDimension = (unDepth == 0) ?  TextureTable::ResourceDesc::DIMENSION::TEXTURE2D : TextureTable::ResourceDesc::DIMENSION::TEXTURE3D;
		}
		else {
			srvDesc.eDimension = (unArraySize == 6) ? TextureTable::ResourceDesc::DIMENSION::TEXTURECUBE
				                                    : TextureTable::ResourceDesc::DIMENSION::TEXTURE2DARRAY;
		}
		SRVHandle = m_SRVTextureTable.Register(
			strTextureName,
			pTexture,
			&srvDesc,
			&dxgiSRVUAVFormat, 
			sizeof(DXGI_FORMAT));

		if (!SRVHandle.IsValid()) {
			OutputDebugStringA(std::format("Failed to load texture SRV : {}", strTextureName).c_str());
			return { {}, {} };
		}

		pTexture->m_d3dSRVDesc = srvDesc.srv;
		pTexture->m_un64RuntimeSRVID = SRVHandle.GetID();
		pTexture->m_d3dSRVHandle = m_SRVTextureTable.GetCPUHandleByHandle(SRVHandle);

		// UAV
		TextureTable::ResourceDesc uavDesc;
		uavDesc.eType = TextureTable::ResourceDesc::TYPE::UAV;
		uavDesc.eDimension = (unArraySize == 1) ? (unDepth == 0) ? TextureTable::ResourceDesc::DIMENSION::TEXTURE2D
			                                                     : TextureTable::ResourceDesc::DIMENSION::TEXTURE3D
			                                    : TextureTable::ResourceDesc::DIMENSION::TEXTURE2DARRAY;

		if (uavDesc.eDimension == TextureTable::ResourceDesc::DIMENSION::TEXTURE3D) {
			uavDesc.pAdditionalData = (void*)(&unDepth);
		}

		UAVHandle = m_UAVTextureTable.Register(
			strTextureName,
			pTexture,
			&uavDesc,
			&dxgiSRVUAVFormat,
			sizeof(DXGI_FORMAT));

		if (!UAVHandle.IsValid()) {
			OutputDebugStringA(std::format("Failed to load texture UAV : {}", strTextureName).c_str());
			return { {}, {} };
		}

		pTexture->m_d3dUAVDesc = uavDesc.uav;
		pTexture->m_un64RuntimeUAVID = UAVHandle.GetID();
		pTexture->m_d3dUAVHandle = m_UAVTextureTable.GetCPUHandleByHandle(UAVHandle);
	}

	return { SRVHandle, UAVHandle };
}

TextureRef<RWRenderTargetTexture> TextureManager::LoadRWRenderTargetTexture(const std::string& strTextureName, uint32 unWidth, uint32 unHeight, DXGI_FORMAT dxgiSRVFormat, DXGI_FORMAT dxgiRTVFormat, DXGI_FORMAT dxgiUAVFormat)
{
	{
		std::lock_guard tableLock{ m_mtxTextureLoad };
		TextureHandle SRVFindHandle = m_SRVTextureTable.GetHandle(strTextureName);
		if (SRVFindHandle.IsValid()) {
			TextureHandle RTVFindHandle = m_RTVTextureTable.GetHandle(strTextureName);
			TextureHandle UAVFindHandle = m_UAVTextureTable.GetHandle(strTextureName);
			return { SRVFindHandle, RTVFindHandle, UAVFindHandle };
		}
	}

	auto pLoadMutex = GetTextureLoadMutex(strTextureName);
	std::lock_guard keyLock{ *pLoadMutex };

	{
		std::lock_guard tableLock{ m_mtxTextureLoad };
		TextureHandle SRVFindHandle = m_SRVTextureTable.GetHandle(strTextureName);
		if (SRVFindHandle.IsValid()) {
			TextureHandle RTVFindHandle = m_RTVTextureTable.GetHandle(strTextureName);
			TextureHandle UAVFindHandle = m_UAVTextureTable.GetHandle(strTextureName);
			return { SRVFindHandle, RTVFindHandle, UAVFindHandle };
		}
	}

	std::shared_ptr<RWRenderTargetTexture> pTexture = std::make_shared<RWRenderTargetTexture>();
	bool bResult = pTexture->Initialize(unWidth, unHeight, dxgiSRVFormat, dxgiRTVFormat, dxgiUAVFormat);
	if (!bResult) {
		return { {}, {} };
	}

	TextureHandle SRVHandle;
	TextureHandle RTVHandle;
	TextureHandle UAVHandle;
	{
		std::lock_guard tableLock{ m_mtxTextureLoad };

		// SRV
		{
			TextureTable::ResourceDesc srvDesc;
			srvDesc.eType = TextureTable::ResourceDesc::TYPE::SRV;
			srvDesc.eDimension = TextureTable::ResourceDesc::DIMENSION::TEXTURE2D;
			SRVHandle = m_SRVTextureTable.Register(
				strTextureName,
				pTexture,
				&srvDesc,
				&dxgiSRVFormat,
				sizeof(DXGI_FORMAT));

			if (!SRVHandle.IsValid()) {
				OutputDebugStringA(std::format("Failed to load texture SRV : {}", strTextureName).c_str());
				return { {}, {} };
			}

			pTexture->m_d3dSRVDesc = srvDesc.srv;
			pTexture->m_un64RuntimeSRVID = SRVHandle.GetID();
			pTexture->m_d3dSRVHandle = m_SRVTextureTable.GetCPUHandleByHandle(SRVHandle);
		}


		// RTV
		{
			TextureTable::ResourceDesc rtvDesc;
			rtvDesc.eType = TextureTable::ResourceDesc::TYPE::RTV;
			rtvDesc.eDimension = TextureTable::ResourceDesc::DIMENSION::TEXTURE2D;
			RTVHandle = m_RTVTextureTable.Register(
				strTextureName,
				pTexture,
				&rtvDesc,
				&dxgiRTVFormat,
				sizeof(DXGI_FORMAT));

			if (!RTVHandle.IsValid()) {
				OutputDebugStringA(std::format("Failed to load texture RTV : {}", strTextureName).c_str());
				return { {}, {} };
			}

			pTexture->m_d3dRTVDesc = rtvDesc.rtv;
			pTexture->m_un64RuntimeRTVID = RTVHandle.GetID();
			pTexture->m_d3dRTVHandle = m_RTVTextureTable.GetCPUHandleByHandle(RTVHandle);
		}

		// UAV
		{
			TextureTable::ResourceDesc uavDesc;
			uavDesc.eType = TextureTable::ResourceDesc::TYPE::UAV;
			uavDesc.eDimension = TextureTable::ResourceDesc::DIMENSION::TEXTURE2D;

			UAVHandle = m_UAVTextureTable.Register(
				strTextureName,
				pTexture,
				&uavDesc,
				&dxgiUAVFormat,
				sizeof(DXGI_FORMAT));

			if (!UAVHandle.IsValid()) {
				OutputDebugStringA(std::format("Failed to load texture UAV : {}", strTextureName).c_str());
				return { {}, {} };
			}

			pTexture->m_d3dUAVDesc = uavDesc.uav;
			pTexture->m_un64RuntimeUAVID = UAVHandle.GetID();
			pTexture->m_d3dUAVHandle = m_UAVTextureTable.GetCPUHandleByHandle(UAVHandle);
		}
	}

	return { SRVHandle, RTVHandle, UAVHandle };
}

std::shared_ptr<Texture> TextureManager::GetTextureByName(const std::string& strTextureName, TEXTURE_RESOURCE_TYPE eResourceType) const
{
	std::shared_ptr<Texture> pTexture = nullptr;

	switch (eResourceType)
	{
	case TEXTURE_RESOURCE_TYPE::SRV:
	{
		pTexture = m_SRVTextureTable.GetResourceByName(strTextureName);
		break;
	}
	case TEXTURE_RESOURCE_TYPE::RTV:
	{
		pTexture = m_RTVTextureTable.GetResourceByName(strTextureName);
		break;
	}
	case TEXTURE_RESOURCE_TYPE::UAV:
	{
		pTexture = m_UAVTextureTable.GetResourceByName(strTextureName);
		break;
	}
	case TEXTURE_RESOURCE_TYPE::DSV:
	{
		pTexture = m_DSVTextureTable.GetResourceByName(strTextureName);
		break;
	}
	default:
	{
		std::unreachable();
	}
	}

	return pTexture;
}

std::shared_ptr<Texture> TextureManager::GetTextureByHandle(const TextureHandle& handle, TEXTURE_RESOURCE_TYPE eResourceType) const
{
	std::shared_ptr<Texture> pTexture = nullptr;

	switch (eResourceType)
	{
	case TEXTURE_RESOURCE_TYPE::SRV:
	{
		pTexture = m_SRVTextureTable.GetResourceByHandle(handle);
		break;
	}
	case TEXTURE_RESOURCE_TYPE::RTV:
	{
		pTexture = m_RTVTextureTable.GetResourceByHandle(handle);
		break;
	}
	case TEXTURE_RESOURCE_TYPE::UAV:
	{
		pTexture = m_UAVTextureTable.GetResourceByHandle(handle);
		break;
	}
	case TEXTURE_RESOURCE_TYPE::DSV:
	{
		pTexture = m_DSVTextureTable.GetResourceByHandle(handle);
		break;
	}
	default:
	{
		std::unreachable();
	}
	}

	return pTexture;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE TextureManager::GetCPUHandleByHandle(const TextureHandle& handle, TEXTURE_RESOURCE_TYPE eResourceType) const
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE CPUHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE{};

	switch (eResourceType)
	{
	case TEXTURE_RESOURCE_TYPE::SRV:
	{
		CPUHandle = m_SRVTextureTable.GetCPUHandleByHandle(handle);
		break;
	}
	case TEXTURE_RESOURCE_TYPE::RTV:
	{
		CPUHandle = m_RTVTextureTable.GetCPUHandleByHandle(handle);
		break;
	}
	case TEXTURE_RESOURCE_TYPE::UAV:
	{
		CPUHandle = m_UAVTextureTable.GetCPUHandleByHandle(handle);
		break;
	}
	case TEXTURE_RESOURCE_TYPE::DSV:
	{
		CPUHandle = m_DSVTextureTable.GetCPUHandleByHandle(handle);
		break;
	}
	default:
	{
		std::unreachable();
	}
	}

	return CPUHandle;
}

void TextureManager::WaitForCopyComplete()
{
	WaitForGPUComplete();
	ReleaseCompletedUploadBuffers();
}

void TextureManager::PollCopyComplete()
{
	ReleaseCompletedUploadBuffers();
}

bool TextureManager::IsCopyComplete()
{
	ReleaseCompletedUploadBuffers();
	const uint64 un64LastSubmittedFenceValue = GetLastSubmittedFenceValue();
	return m_pd3dFence->GetCompletedValue() >= un64LastSubmittedFenceValue;
}

bool TextureManager::IsFenceComplete(uint64 ui64FenceValue) const
{
	if (ui64FenceValue == 0) {
		return true;
	}

	return m_pd3dFence->GetCompletedValue() >= ui64FenceValue;
}

uint64 TextureManager::GetLastSubmittedFenceValue() const
{
	std::lock_guard submitLock{ m_mtxSubmit };
	return m_un64FenceValue;
}

uint64 TextureManager::GetPendingCopyFenceValue() const
{
	const uint64 un64LastSubmittedFenceValue = GetLastSubmittedFenceValue();
	const uint64 un64CompletedFenceValue = m_pd3dFence->GetCompletedValue();
	if (un64CompletedFenceValue >= un64LastSubmittedFenceValue) {
		return 0;
	}

	return un64LastSubmittedFenceValue;
}

void TextureManager::CreateUploadBuffer(ID3D12Resource** ppUploadBuffer, uint32 unBytes)
{
	HRESULT hr = DEVICE->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(unBytes),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(ppUploadBuffer)
	);

	if (FAILED(hr)) {
		__debugbreak();
	}
}

std::shared_ptr<std::mutex> TextureManager::GetTextureLoadMutex(const std::string& strTextureName)
{
	auto it = m_TextureLoadMutexRegistry.find(strTextureName);
	if (it != m_TextureLoadMutexRegistry.end()) {
		return it->second;
	}

	auto pNewMutex = std::make_shared<std::mutex>();
	auto [newIt, bInserted] = m_TextureLoadMutexRegistry.insert({ strTextureName, pNewMutex });

	return newIt->second;
}

void TextureManager::ReleaseCompletedUploadBuffers()
{
	const uint64 un64CompletedValue = m_pd3dFence->GetCompletedValue();
	m_CommandListPool.ReclaimEnded(un64CompletedValue);
}

void TextureManager::UpdateResources(ComPtr<ID3D12Resource> pResource, D3D12_RESOURCE_STATES d3dCurrentState, const std::vector<D3D12_SUBRESOURCE_DATA>& subResources, uint32 unBytes, ComPtr<ID3D12Resource> pd3dUploadBuffer)
{
	if (!pd3dUploadBuffer) {
		CreateUploadBuffer(pd3dUploadBuffer.GetAddressOf(), unBytes);
	}

	// BinaryResource -> Upload Buffer -> Texture Buffer
	auto cmdList = AllocateCommandListSafe();
	{
		cmdList->pd3dCommandList->ResourceBarrier(
			1,
			&CD3DX12_RESOURCE_BARRIER::Transition(
				pResource.Get(), 
				d3dCurrentState,
				D3D12_RESOURCE_STATE_COPY_DEST, 
				D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, 
				D3D12_RESOURCE_BARRIER_FLAG_NONE)
		);
		
		::UpdateSubresources(cmdList->pd3dCommandList.Get(), pResource.Get(), pd3dUploadBuffer.Get(), 0, 0, subResources.size(), subResources.data());
		
		cmdList->pd3dCommandList->ResourceBarrier(
			1,
			&CD3DX12_RESOURCE_BARRIER::Transition(
				pResource.Get(),
				D3D12_RESOURCE_STATE_COPY_DEST,
				d3dCurrentState,
				D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
				D3D12_RESOURCE_BARRIER_FLAG_NONE)
		);
	}
	cmdList->AddPendingUploadBuffer(pd3dUploadBuffer);
	// 목적지 텍스처도 카피 완료(펜스)까지 수명 고정 — 씬 전환 등으로 먼저 해제되면
	// GPU 카피가 해제된 메모리에 쓰다 페이지폴트(Device Removed)
	cmdList->AddPendingUploadBuffer(pResource);
	ExcuteCommandList(*cmdList);
}

#pragma region D3D
void TextureManager::CreateCommandList()
{
	HRESULT hr{};

	// Create Command Queue
	D3D12_COMMAND_QUEUE_DESC d3dCommandQueueDesc{};
	::ZeroMemory(&d3dCommandQueueDesc, sizeof(D3D12_COMMAND_QUEUE_DESC));
	{
		d3dCommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		d3dCommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	}
	hr = m_pd3dDevice->CreateCommandQueue(&d3dCommandQueueDesc, IID_PPV_ARGS(m_pd3dCommandQueue.GetAddressOf()));
	if (FAILED(hr)) {
		SHOW_ERROR("Failed to create CommandQueue");
	}
}

void TextureManager::CreateFence()
{
	HRESULT hr{};

	hr = m_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_pd3dFence.GetAddressOf()));
	if (FAILED(hr)) {
		SHOW_ERROR("Failed to create fence");
	}

	m_hFenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
}

void TextureManager::ExcuteCommandList(CommandListPair& cmdPair)
{
	HRESULT hr = cmdPair.pd3dCommandList->Close();
	if (FAILED(hr)) {
		SHOW_ERROR("Failed to close CommandList");
		__debugbreak();
	}

	{
		std::lock_guard submitLock{ m_mtxSubmit };

		ID3D12CommandList* ppCommandLists[] = { cmdPair.pd3dCommandList.Get() };
		m_pd3dCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
		const uint64 un64FenceValue = Fence();
		if (!m_CommandListPool.MarkSubmitted(cmdPair, un64FenceValue)) {
			assert(false && "Failed to mark cmdlist");
		}
	}
}

uint64 TextureManager::Fence()
{
	m_un64FenceValue++;
	m_pd3dCommandQueue->Signal(m_pd3dFence.Get(), m_un64FenceValue);

	return m_un64FenceValue;
}

void TextureManager::WaitForGPUComplete()
{
	const uint64 un64ExpectedFenceValue = GetLastSubmittedFenceValue();
	if (m_pd3dFence->GetCompletedValue() >= un64ExpectedFenceValue) {
		return;
	}

	{
		std::lock_guard eventLock{ m_mtxFence };

		if (m_pd3dFence->GetCompletedValue() < un64ExpectedFenceValue)
		{
			m_pd3dFence->SetEventOnCompletion(un64ExpectedFenceValue, m_hFenceEvent);
			::WaitForSingleObject(m_hFenceEvent, INFINITE);
		}
	}
}

CommandListPair* TextureManager::AllocateCommandListSafe()
{
	while (true) {
		const uint64 un64CompletedFenceValue = m_pd3dFence->GetCompletedValue();
		CommandListPair* pCmdList = m_CommandListPool.Allocate(un64CompletedFenceValue);

		if (pCmdList) {
			return pCmdList;
		}

		// Wait for GPU complete when cmdlist pool is full
		WaitForGPUComplete();
		std::this_thread::yield();
	}
}


#pragma endregion D3D
