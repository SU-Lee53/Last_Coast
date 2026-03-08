#include "pch.h"
#include "Skybox.h"
#include "ComputePass.h"


void Skybox::Initialize(const std::string& strDay)
{
	// TODO : 해결사항
	// 1. 현재 TEXTURE2DARRAY 형태의 SRV/UAV 를 만들 수 없고 현재 구조상 딱히 구분할 방법도 없음
	// 2. HDRI 이미지가 Upload Heap 을 거쳐 Copy가 완료되기 이전에 Compute 가 돌아버림

	const int32 nWidth = 512;
	const int32 nHeight = 512;

	const std::string strTextureBasePath = "../Resources/Skybox/";
	auto id = TEXTURE->LoadTexture(strTextureBasePath + strDay);
	auto [srv, uav] = TEXTURE->LoadUnorderedAccessTexture("Skybox_Day", 6, nWidth, nHeight, DXGI_FORMAT_R32G32B32A32_FLOAT);
	m_SkyboxSRVID = srv;
	m_SkyboxUAVID = uav;

	m_pPass = std::make_shared<HDRIToCubeMapPass>();
	m_pPass->Initialize();

	auto pHDRI = TEXTURE->GetTextureByID(id, TEXTURE_RESOURCE_TYPE::SRV);
	auto pUAV = static_pointer_cast<UnorderedAccessTexture>(TEXTURE->GetTextureByID(uav, TEXTURE_RESOURCE_TYPE::UAV));

	D3D12_RESOURCE_DESC HDRITextureDesc = pHDRI->GetResource()->GetDesc();
	HDRIToCubeMapPass::CB_SKYBOX_SIZE cbData;
	cbData.nWidth = nWidth;
	cbData.nHeight = nHeight;

	ComputePassInput input;
	
	input.pSRVs.push_back(pHDRI);
	input.pUAVs.push_back(pUAV);
	input.pAdditionalContext = (void*)(&cbData);

	m_pPass->Dispatch(input, nWidth / 8, nHeight / 8, 6);
}
