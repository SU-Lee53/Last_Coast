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

	Load<StandardShader>();
	Load<AnimatedShader>();
	Load<TerrainShader>();
	Load<WaterShader>();
	//Load<FullScreenShader>();
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
		L"-Zi",
		L"-Qembed_debug",
		L"-Od",
		L"-Zss",
		L"-Zpc",
		//L"-Wall",
#else
		L"-O3",
		L"-Qstrip_debug",
		L"-Qstrip_reflect",
		//L"-Wall",
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
		return D3D12_SHADER_BYTECODE{};
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
	// Common quad vs
	Compile("QuadVS", L"NewCommon.hlsl", L"VSQuad", SHADER_TYPE::VS);

	// Shaders.hlsl
	Compile("StandardVS", L"DefferedShader.hlsl", L"VSStandard", SHADER_TYPE::VS);
	Compile("AnimatedVS", L"DefferedShader.hlsl", L"VSAnimated", SHADER_TYPE::VS);
	Compile("GBufferOpaquePS", L"DefferedShader.hlsl", L"PSGBufferOpaque", SHADER_TYPE::PS);
	Compile("GBufferAlphaMaskPS", L"DefferedShader.hlsl", L"PSGBufferAlphaMask", SHADER_TYPE::PS);
	Compile("GBufferWaterPS", L"DefferedShader.hlsl", L"PSGBufferWater", SHADER_TYPE::PS);
	
	//Compile("StandardPS", L"DefferedShader.hlsl", L"PSStandard", SHADER_TYPE::PS);
	//Compile("AnimatedPS", L"DefferedShader.hlsl", L"PSAnimated", SHADER_TYPE::PS);
	
	Compile("TerrainVS", L"DefferedShader.hlsl", L"VSTerrain", SHADER_TYPE::VS);
	Compile("TerrainInstancedVS", L"DefferedShader.hlsl", L"VSTerrainInstanced", SHADER_TYPE::VS);
	Compile("TerrainPS", L"DefferedShader.hlsl", L"PSTerrain", SHADER_TYPE::PS);
	
	//Compile("LightingVS", L"DefferedShader.hlsl", L"VSDefferedLighting", SHADER_TYPE::VS);
	Compile("LightingPS", L"DefferedShader.hlsl", L"PSDefferedLighting", SHADER_TYPE::PS);
	
	//Compile("ToneMappingVS", L"ToneMapping.hlsl", L"VSToneMapping", SHADER_TYPE::VS);
	Compile("ToneMappingPS", L"ToneMapping.hlsl", L"PSToneMapping", SHADER_TYPE::PS);
	
	Compile("SkyboxVS", L"Skybox.hlsl", L"VSSkybox", SHADER_TYPE::VS);
	Compile("SkyboxPS", L"Skybox.hlsl", L"PSSkybox", SHADER_TYPE::PS);
	Compile("CelestialDiskPS", L"Skybox.hlsl", L"PSCelestialDisk", SHADER_TYPE::PS);

	Compile("ForwardStandardVS", L"ForwardShader.hlsl", L"VSForwardStandard", SHADER_TYPE::VS);
	Compile("ForwardAnimatedVS", L"ForwardShader.hlsl", L"VSForwardAnimated", SHADER_TYPE::VS);
	Compile("ForwardLightingPS", L"ForwardShader.hlsl", L"PSForwardLighting", SHADER_TYPE::PS);

	Compile("ShadowStandardVS", L"ShadowMapShader.hlsl", L"VSShadowStandard", SHADER_TYPE::VS);
	Compile("ShadowTerrainVS", L"ShadowMapShader.hlsl", L"VSShadowTerrain", SHADER_TYPE::VS);
	Compile("ShadowTerrainInstancedVS", L"ShadowMapShader.hlsl", L"VSShadowTerrainInstanced", SHADER_TYPE::VS);
	Compile("ShadowAnimatedVS", L"ShadowMapShader.hlsl", L"VSShadowAnimated", SHADER_TYPE::VS);

	Compile("UIRectVS", L"Sprite.hlsl", L"VSUIRect", SHADER_TYPE::VS);
	Compile("UISpritePS", L"Sprite.hlsl", L"PSUISprite", SHADER_TYPE::PS);

	// Particle
	Compile("ParticleVS", L"Particle.hlsl", L"VSParticle", SHADER_TYPE::VS);
	Compile("ParticlePS", L"Particle.hlsl", L"PSParticle", SHADER_TYPE::PS);

	// PostProcess
	//Compile("DefferedFogVS", L"HDRPostProcessing.hlsl", L"VSDefferedFog", SHADER_TYPE::VS);
	Compile("DefferedFogPS", L"HDRPostProcessing.hlsl", L"PSDefferedFog", SHADER_TYPE::PS);
	Compile("AtmosphericFogDetailPS", L"AtmosphericFogDetail.hlsl", L"PSAtmosphericFogDetail", SHADER_TYPE::PS);

	Compile("SSAOPS", L"SSAO.hlsl", L"PSSSAO", SHADER_TYPE::PS);
	Compile("SSAOBilateralBlurPS", L"SSAO.hlsl", L"PSSSAOBilateralBlur", SHADER_TYPE::PS);
	Compile("LightShaftPS", L"LightShaft.hlsl", L"PSLightShaft", SHADER_TYPE::PS);

	Compile("DebugLineVS", L"DebugLineShader.hlsl", L"VSDebugLine", SHADER_TYPE::VS);
	Compile("DebugLineGreenPS", L"DebugLineShader.hlsl", L"PSDebugLineGreen", SHADER_TYPE::PS);
	Compile("DebugLineRedPS", L"DebugLineShader.hlsl", L"PSDebugLineRed", SHADER_TYPE::PS);

	// Compute
	Compile("HDRIToCubeMapCS", L"HDRIToCubeMap.hlsl", L"CSHDRIToCubeMap", SHADER_TYPE::CS);

	Compile("ToneMapLUTCS", L"LUTBaking.hlsl", L"CSToneMapLUT", SHADER_TYPE::CS);
	Compile("GradingLUTCS", L"LUTBaking.hlsl", L"CSGradingLUT", SHADER_TYPE::CS);

	Compile("BloomBrightExtractCS", L"Bloom.hlsl", L"CSBrightExtractDownsample", SHADER_TYPE::CS);
	Compile("BloomBlurHorzCS", L"Bloom.hlsl", L"CSBloomBlurHorizontal", SHADER_TYPE::CS);
	Compile("BloomBlurVertCS", L"Bloom.hlsl", L"CSBloomBlurVertical", SHADER_TYPE::CS);

	Compile("ExtractLuminanceCS", L"AutoExposure.hlsl", L"CSExtractLuminance", SHADER_TYPE::CS);
	Compile("ReduceLuminanceCS", L"AutoExposure.hlsl", L"CSReduceLuminance", SHADER_TYPE::CS);
	Compile("FinalLuminanceCS", L"AutoExposure.hlsl", L"CSFinalLuminance", SHADER_TYPE::CS);

}
