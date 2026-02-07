#include "pch.h"
#include "ShaderManager.h"

const std::wstring ShaderManager::g_wstrShaderPath = L"../HLSL/";
const std::string ShaderManager::g_strShaderPath = "../HLSL/";

ShaderManager::~ShaderManager()
{
	OutputDebugStringA("ShaderManager Destroy\n");
	OutputDebugStringA("==========================================================================================\n");
	OutputDebugStringA(std::format("FullScreenShader Ref Count : {}\n", m_pShaderMap[typeid(FullScreenShader)].use_count()).c_str());
	OutputDebugStringA(std::format("StandardShader Ref Count : {}\n", m_pShaderMap[typeid(StandardShader)].use_count()).c_str());
	OutputDebugStringA("==========================================================================================\n");
}

void ShaderManager::Initialize(ComPtr<ID3D12Device> pDevice)
{
	m_pd3dDevice = pDevice;

	if (!m_pdxcUtils) {
		DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(m_pdxcUtils.GetAddressOf()));
	}

	if (!m_pdxcCompiler) {
		DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(m_pdxcCompiler.GetAddressOf()));
	}

	CompileShaders();

	Load<FullScreenShader>();
	Load<StandardShader>();
	Load<AnimatedShader>();
	Load<TerrainShader>();
}

D3D12_SHADER_BYTECODE ShaderManager::GetShaderByteCode(const std::string& strShaderName)
{
	auto it = m_pCompiledShaderByteCodeMap.find(strShaderName);
	if (it != m_pCompiledShaderByteCodeMap.end()) {
		return it->second.first;
	}

	return D3D12_SHADER_BYTECODE{};
}

void ShaderManager::ReleaseBlobs()
{
	for (auto& pBlob : m_pd3dBlobs) {
		pBlob.Reset();
		pBlob = nullptr;
	}

	m_pd3dBlobs.clear();
}

D3D12_SHADER_BYTECODE ShaderManager::CompileShaderDXC(const std::wstring& wstrFileName, const std::wstring& wstrShaderEntry, const std::wstring& wstrShaderProfile, IDxcBlob** ppBlob)
{
	HRESULT hr{};

	if (!m_pdxcUtils) {
		DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(m_pdxcUtils.GetAddressOf()));
	}

	if (!m_pdxcCompiler) {
		DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(m_pdxcCompiler.GetAddressOf()));
	}

	const std::wstring wstrFilePath = g_wstrShaderPath + wstrFileName;

	ComPtr<IDxcBlobEncoding> pSourceBlob;
	m_pdxcUtils->LoadFile(wstrFilePath.c_str(), nullptr, pSourceBlob.GetAddressOf());
	if (FAILED(hr)) {
		MessageBoxW(WinCore::g_hWnd, (L"Failed to open : " + wstrFilePath).c_str(), wstrFileName.c_str(), 0);
		__debugbreak();
		return D3D12_SHADER_BYTECODE{};
	}

	DxcBuffer dxcBuffer{};
	dxcBuffer.Ptr = pSourceBlob->GetBufferPointer();
	dxcBuffer.Size = pSourceBlob->GetBufferSize();
	dxcBuffer.Encoding = DXC_CP_UTF8;

	std::vector<LPCWSTR> lpwstrArgs = {
		L"-E", wstrShaderEntry.c_str(),
		L"-T", wstrShaderProfile.c_str(),
		L"-I", L"../HLSL",
#ifdef _DEBUG
		L"-Zi", L"Qembed_debug",
		L"-Od"
#endif
	};

	ComPtr<IDxcIncludeHandler> pdxcIncludeHandler;
	m_pdxcUtils->CreateDefaultIncludeHandler(pdxcIncludeHandler.GetAddressOf());

	// Compile
	ComPtr<IDxcResult> pdxcResult = nullptr;
	hr = m_pdxcCompiler->Compile(
		&dxcBuffer,
		lpwstrArgs.data(),
		(UINT32)lpwstrArgs.size(),
		pdxcIncludeHandler.Get(),
		IID_PPV_ARGS(pdxcResult.GetAddressOf()));

	if (FAILED(hr)) {
		MessageBoxW(WinCore::g_hWnd, (L"Failed to compile : " + wstrFilePath).c_str(), wstrFileName.c_str(), 0);
		__debugbreak();
		return D3D12_SHADER_BYTECODE{};
	}

	ComPtr<IDxcBlobUtf8> pdxcErrorBlob = nullptr;
	pdxcResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(pdxcErrorBlob.GetAddressOf()), nullptr);
	if (pdxcErrorBlob && pdxcErrorBlob->GetStringLength() > 0) {
		OutputDebugStringA(pdxcErrorBlob->GetStringPointer());
		MessageBoxA(WinCore::g_hWnd, pdxcErrorBlob->GetStringPointer(), "DXC ERROR", 0);
		__debugbreak();
		return D3D12_SHADER_BYTECODE{};;
	}

	pdxcResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(ppBlob), nullptr);
	D3D12_SHADER_BYTECODE d3dByteCode{};
	d3dByteCode.BytecodeLength = (*ppBlob)->GetBufferSize();
	d3dByteCode.pShaderBytecode = (*ppBlob)->GetBufferPointer();

	return d3dByteCode;
}

void ShaderManager::CompileShaders()
{
	enum class SHADER_TYPE { VS, HS, DS, GS, PS, CS };

	const auto Compile = [&](const std::string& strKey, const std::wstring& wstrFilename, const std::wstring& wstrEntry, SHADER_TYPE eShaderType) {
		ComPtr<IDxcBlob> pBlob;
		D3D12_SHADER_BYTECODE d3dByteCode{};
		switch (eShaderType) {
		case SHADER_TYPE::VS:
		{
			d3dByteCode = CompileShaderDXC(wstrFilename, wstrEntry, L"vs_6_1", pBlob.GetAddressOf());
			break;
		}
		case SHADER_TYPE::HS:
		{
			d3dByteCode = CompileShaderDXC(wstrFilename, wstrEntry, L"hs_6_1", pBlob.GetAddressOf());
			break;
		}
		case SHADER_TYPE::DS:
		{
			d3dByteCode = CompileShaderDXC(wstrFilename, wstrEntry, L"ds_6_1", pBlob.GetAddressOf());
			break;
		}
		case SHADER_TYPE::GS:
		{
			d3dByteCode = CompileShaderDXC(wstrFilename, wstrEntry, L"gs_6_1", pBlob.GetAddressOf());
			break;
		}
		case SHADER_TYPE::PS:
		{
			d3dByteCode = CompileShaderDXC(wstrFilename, wstrEntry, L"ps_6_1", pBlob.GetAddressOf());
			break;
		}
		case SHADER_TYPE::CS:
		{
			d3dByteCode = CompileShaderDXC(wstrFilename, wstrEntry, L"cs_6_1", pBlob.GetAddressOf());
			break;
		}
		default:
		{
			std::unreachable();
		}
		}

		m_pCompiledShaderByteCodeMap.insert({ strKey, { d3dByteCode, pBlob } });
	};

	// Shaders.hlsl
	Compile("StandardVS", L"Shaders.hlsl", L"VSStandard", SHADER_TYPE::VS);
	Compile("StandardPS", L"Shaders.hlsl", L"PSStandard", SHADER_TYPE::PS);
	
	Compile("AnimatedVS", L"Shaders.hlsl", L"VSAnimated", SHADER_TYPE::VS);
	Compile("AnimatedPS", L"Shaders.hlsl", L"PSAnimated", SHADER_TYPE::PS);
	
	Compile("TerrainVS", L"Shaders.hlsl", L"VSTerrain", SHADER_TYPE::VS);
	Compile("TerrainPS", L"Shaders.hlsl", L"PSTerrain", SHADER_TYPE::PS);

	// FullScreenShader.hlsl
	Compile("FullScreenVS", L"FullScreenShader.hlsl", L"VSFullScreen", SHADER_TYPE::VS);
	Compile("FullScreenPS", L"FullScreenShader.hlsl", L"PSFullScreen", SHADER_TYPE::PS);
	
	// EffectShader.hlsl
	Compile("RayVS", L"EffectShader.hlsl", L"VSRay", SHADER_TYPE::VS);
	Compile("RayGS", L"EffectShader.hlsl", L"GSRay", SHADER_TYPE::GS);
	Compile("RayPS", L"EffectShader.hlsl", L"PSRay", SHADER_TYPE::PS);
	
	Compile("ExplosionVS", L"EffectShader.hlsl", L"VSExplosion", SHADER_TYPE::VS);
	Compile("ExplosionGS", L"EffectShader.hlsl", L"GSExplosion", SHADER_TYPE::GS);
	Compile("ExplosionPS", L"EffectShader.hlsl", L"PSExplosion", SHADER_TYPE::PS);

	// Sprite.hlsl
	Compile("TextureSpriteVS", L"Sprite.hlsl", L"VSTextureSprite", SHADER_TYPE::VS);
	Compile("TextureSpriteGS", L"Sprite.hlsl", L"GSTextureSprite", SHADER_TYPE::GS);
	Compile("TextureSpritePS", L"Sprite.hlsl", L"PSTextureSprite", SHADER_TYPE::PS);
	
	Compile("TextSpriteVS", L"Sprite.hlsl", L"VSTextSprite", SHADER_TYPE::VS);
	Compile("TextSpriteGS", L"Sprite.hlsl", L"GSTextSprite", SHADER_TYPE::GS);
	Compile("TextSpritePS", L"Sprite.hlsl", L"PSTextSprite", SHADER_TYPE::PS);
	
	Compile("BillboardSpriteVS", L"Sprite.hlsl", L"VSBillboardSprite", SHADER_TYPE::VS);
	Compile("BillboardSpriteGS", L"Sprite.hlsl", L"GSBillboardSprite", SHADER_TYPE::GS);
	Compile("BillboardSpritePS", L"Sprite.hlsl", L"PSBillboardSprite", SHADER_TYPE::PS);
}
