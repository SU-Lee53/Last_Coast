#include "pch.h"
#include "ShaderManager.h"


ShaderManager::ShaderManager(ComPtr<ID3D12Device> pDevice)
{
	m_pd3dDevice = pDevice;
}

ShaderManager::~ShaderManager()
{
	OutputDebugStringA("ShaderManager Destroy\n");
	OutputDebugStringA("==========================================================================================");
	OutputDebugStringA(std::format("DiffusedShader Ref Count : {}\n", m_pShaderMap[typeid(DiffusedShader)].use_count()).c_str());
	OutputDebugStringA(std::format("FullScreenShader Ref Count : {}\n", m_pShaderMap[typeid(FullScreenShader)].use_count()).c_str());
	OutputDebugStringA(std::format("TexturedShader Ref Count : {}\n", m_pShaderMap[typeid(TexturedShader)].use_count()).c_str());
	OutputDebugStringA(std::format("TexturedIlluminatedShader Ref Count : {}\n", m_pShaderMap[typeid(TexturedIlluminatedShader)].use_count()).c_str());
	OutputDebugStringA(std::format("TexturedNormalShader Ref Count : {}\n", m_pShaderMap[typeid(TexturedNormalShader)].use_count()).c_str());
	OutputDebugStringA("==========================================================================================");
}

void ShaderManager::Initialize()
{
	CompileShaders();

	Load<DiffusedShader>();
	Load<FullScreenShader>();
	Load<TexturedShader>();
	Load<TexturedIlluminatedShader>();
	Load<TexturedNormalShader>();
	//Load<PlayerShader>();
}

D3D12_SHADER_BYTECODE ShaderManager::GetShaderByteCode(const std::string& strShaderName)
{
	auto it = m_pCompiledShaderByteCodeMap.find(strShaderName);
	if (it != m_pCompiledShaderByteCodeMap.end()) {
		return it->second;
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

void ShaderManager::CompileShaders()
{
	ComPtr<ID3DBlob> m_pd3dVSBlob;
	ComPtr<ID3DBlob> m_pd3dGSBlob;
	ComPtr<ID3DBlob> m_pd3dPSBlob;

	// Shaders.hlsl
	m_pCompiledShaderByteCodeMap.insert({ "VSDiffused", Shader::CompileShader(L"../HLSL/DiffusedShader.hlsl", "VSDiffusedInstanced", "vs_5_1", m_pd3dVSBlob.GetAddressOf()) });
	m_pCompiledShaderByteCodeMap.insert({ "PSDiffused", Shader::CompileShader(L"../HLSL/DiffusedShader.hlsl", "PSDiffused", "ps_5_1", m_pd3dPSBlob.GetAddressOf()) });
	m_pd3dBlobs.push_back(m_pd3dVSBlob);
	m_pd3dBlobs.push_back(m_pd3dPSBlob);

	m_pCompiledShaderByteCodeMap.insert({ "VSTextured", Shader::CompileShader(L"../HLSL/TexturedShader.hlsl", "VSTextured", "vs_5_1", m_pd3dVSBlob.GetAddressOf()) });
	m_pCompiledShaderByteCodeMap.insert({ "PSTextured", Shader::CompileShader(L"../HLSL/TexturedShader.hlsl", "PSTextured", "ps_5_1", m_pd3dPSBlob.GetAddressOf()) });
	m_pd3dBlobs.push_back(m_pd3dVSBlob);
	m_pd3dBlobs.push_back(m_pd3dPSBlob);

	m_pCompiledShaderByteCodeMap.insert({ "TexturedIlluminatedVS", Shader::CompileShader(L"../HLSL/TexturedIlluminatedShader.hlsl", "VSTexturedIlluminated", "vs_5_1", m_pd3dVSBlob.GetAddressOf()) });
	m_pCompiledShaderByteCodeMap.insert({ "TexturedIlluminatedPS", Shader::CompileShader(L"../HLSL/TexturedIlluminatedShader.hlsl", "PSTexturedIlluminated", "ps_5_1", m_pd3dPSBlob.GetAddressOf()) });
	m_pd3dBlobs.push_back(m_pd3dVSBlob);
	m_pd3dBlobs.push_back(m_pd3dGSBlob);
	m_pd3dBlobs.push_back(m_pd3dPSBlob);

	m_pCompiledShaderByteCodeMap.insert({ "TexturedNormalVS", Shader::CompileShader(L"../HLSL/TexturedNormalShader.hlsl", "VSTexturedNormal", "vs_5_1", m_pd3dVSBlob.GetAddressOf()) });
	m_pCompiledShaderByteCodeMap.insert({ "TexturedNormalGS", Shader::CompileShader(L"../HLSL/TexturedNormalShader.hlsl", "PSTexturedNormal", "ps_5_1", m_pd3dGSBlob.GetAddressOf()) });
	m_pd3dBlobs.push_back(m_pd3dVSBlob);
	m_pd3dBlobs.push_back(m_pd3dPSBlob);

	m_pCompiledShaderByteCodeMap.insert({ "FullScreenVS", Shader::CompileShader(L"../HLSL/FullScreenShader.hlsl", "VSFullScreen", "vs_5_1", m_pd3dVSBlob.GetAddressOf()) });
	m_pCompiledShaderByteCodeMap.insert({ "FullScreenPS", Shader::CompileShader(L"../HLSL/FullScreenShader.hlsl", "PSFullScreen", "ps_5_1", m_pd3dGSBlob.GetAddressOf()) });
	m_pd3dBlobs.push_back(m_pd3dVSBlob);
	m_pd3dBlobs.push_back(m_pd3dGSBlob);
	m_pd3dBlobs.push_back(m_pd3dPSBlob);

	m_pCompiledShaderByteCodeMap.insert({ "RayVS", Shader::CompileShader(L"../HLSL/EffectShader.hlsl", "VSRay", "vs_5_1", m_pd3dVSBlob.GetAddressOf()) });
	m_pCompiledShaderByteCodeMap.insert({ "RayGS", Shader::CompileShader(L"../HLSL/EffectShader.hlsl", "GSRay", "gs_5_1", m_pd3dGSBlob.GetAddressOf()) });
	m_pCompiledShaderByteCodeMap.insert({ "RayPS", Shader::CompileShader(L"../HLSL/EffectShader.hlsl", "PSRay", "ps_5_1", m_pd3dPSBlob.GetAddressOf()) });
	m_pd3dBlobs.push_back(m_pd3dVSBlob);
	m_pd3dBlobs.push_back(m_pd3dGSBlob);
	m_pd3dBlobs.push_back(m_pd3dPSBlob);

	m_pCompiledShaderByteCodeMap.insert({ "ExplosionVS", Shader::CompileShader(L"../HLSL/EffectShader.hlsl", "VSExplosion", "vs_5_1", m_pd3dVSBlob.GetAddressOf()) });
	m_pCompiledShaderByteCodeMap.insert({ "ExplosionGS", Shader::CompileShader(L"../HLSL/EffectShader.hlsl", "GSExplosion", "gs_5_1", m_pd3dGSBlob.GetAddressOf()) });
	m_pCompiledShaderByteCodeMap.insert({ "ExplosionPS", Shader::CompileShader(L"../HLSL/EffectShader.hlsl", "PSExplosion", "ps_5_1", m_pd3dPSBlob.GetAddressOf()) });
	m_pd3dBlobs.push_back(m_pd3dVSBlob);
	m_pd3dBlobs.push_back(m_pd3dGSBlob);
	m_pd3dBlobs.push_back(m_pd3dPSBlob);

	m_pCompiledShaderByteCodeMap.insert({ "BillboardVS", Shader::CompileShader(L"../HLSL/BillboardShader.hlsl", "VSBillboard", "vs_5_1", m_pd3dVSBlob.GetAddressOf()) });
	m_pCompiledShaderByteCodeMap.insert({ "BillboardGS", Shader::CompileShader(L"../HLSL/BillboardShader.hlsl", "GSBillboard", "gs_5_1", m_pd3dGSBlob.GetAddressOf()) });
	m_pCompiledShaderByteCodeMap.insert({ "BillboardPS", Shader::CompileShader(L"../HLSL/BillboardShader.hlsl", "PSBillboard", "ps_5_1", m_pd3dPSBlob.GetAddressOf()) });
	m_pd3dBlobs.push_back(m_pd3dVSBlob);
	m_pd3dBlobs.push_back(m_pd3dGSBlob);
	m_pd3dBlobs.push_back(m_pd3dPSBlob);

	// Sprite.hlsl						   
	m_pCompiledShaderByteCodeMap.insert({ "TextureSpriteVS", Shader::CompileShader(L"../HLSL/Sprite.hlsl", "VSTextureSprite", "vs_5_1", m_pd3dVSBlob.GetAddressOf()) });
	m_pCompiledShaderByteCodeMap.insert({ "TextureSpriteGS", Shader::CompileShader(L"../HLSL/Sprite.hlsl", "GSTextureSprite", "gs_5_1", m_pd3dGSBlob.GetAddressOf()) });
	m_pCompiledShaderByteCodeMap.insert({ "TextureSpritePS", Shader::CompileShader(L"../HLSL/Sprite.hlsl", "PSTextureSprite", "ps_5_1", m_pd3dPSBlob.GetAddressOf()) });
	m_pd3dBlobs.push_back(m_pd3dVSBlob);
	m_pd3dBlobs.push_back(m_pd3dGSBlob);
	m_pd3dBlobs.push_back(m_pd3dPSBlob);

	m_pCompiledShaderByteCodeMap.insert({ "TextSpriteVS", Shader::CompileShader(L"../HLSL/Sprite.hlsl", "VSTextSprite", "vs_5_1", m_pd3dVSBlob.GetAddressOf()) });
	m_pCompiledShaderByteCodeMap.insert({ "TextSpriteGS", Shader::CompileShader(L"../HLSL/Sprite.hlsl", "GSTextSprite", "gs_5_1", m_pd3dGSBlob.GetAddressOf()) });
	m_pCompiledShaderByteCodeMap.insert({ "TextSpritePS", Shader::CompileShader(L"../HLSL/Sprite.hlsl", "PSTextSprite", "ps_5_1", m_pd3dPSBlob.GetAddressOf()) });
	m_pd3dBlobs.push_back(m_pd3dVSBlob);
	m_pd3dBlobs.push_back(m_pd3dGSBlob);
	m_pd3dBlobs.push_back(m_pd3dPSBlob);

	m_pCompiledShaderByteCodeMap.insert({ "BillboardSpriteVS", Shader::CompileShader(L"../HLSL/Sprite.hlsl", "VSBillboardSprite", "vs_5_1", m_pd3dVSBlob.GetAddressOf()) });
	m_pCompiledShaderByteCodeMap.insert({ "BillboardSpriteGS", Shader::CompileShader(L"../HLSL/Sprite.hlsl", "GSBillboardSprite", "gs_5_1", m_pd3dGSBlob.GetAddressOf()) });
	m_pCompiledShaderByteCodeMap.insert({ "BillboardSpritePS", Shader::CompileShader(L"../HLSL/Sprite.hlsl", "PSBillboardSprite", "ps_5_1", m_pd3dPSBlob.GetAddressOf()) });
	m_pd3dBlobs.push_back(m_pd3dVSBlob);
	m_pd3dBlobs.push_back(m_pd3dGSBlob);
	m_pd3dBlobs.push_back(m_pd3dPSBlob);

}
