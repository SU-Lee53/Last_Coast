#include "pch.h"
#include "WaterGridObject.h"

void WaterGridObject::Initialize()
{
	if (GetName().empty()) {
		SetName("WaterGridObject");
	}

	CreateNoiseTexture();

	// Setup Grid Mesh
	if (!GetComponent<MeshRenderer>()) {
		MESHLOADINFO meshLoadInfo = GridMesh::CreateLoadInfo(
			64,
			64,
			512_m,
			512_m,
			36.0f,
			MESH_TYPE::WATER);

		MATERIALLOADINFO materialLoadInfo{};
		materialLoadInfo.v4Diffuse = Vector4(0.02f, 0.20f, 0.32f, 1.0f);
		materialLoadInfo.v4Specular = Vector4(1.0f, 1.0f, 1.0f, 0.65f);
		materialLoadInfo.fSmoothness = 0.86f;
		materialLoadInfo.fMetallic = 0.0f;

		AddComponent<MeshRenderer>(
			std::vector<MESHLOADINFO>{ std::move(meshLoadInfo) },
			std::vector<MATERIALLOADINFO>{ materialLoadInfo });
	}

	if (m_NoiseTexture.IsValid()) {
		GetComponent<MeshRenderer>()->SetTexture(m_NoiseTexture, 0, TEXTURE_TYPE::ALBEDO);
	}

	if (!GetComponent<ICollider>()) {
		AddComponent<StaticCollider>();
	}

	StaticObject::Initialize();
}

void WaterGridObject::Update()
{
}

void WaterGridObject::PostUpdate()
{
	for (auto& component : m_pComponents) {
		if (component) {
			component->Update();
		}
	}

	for (auto& pChild : m_pChildren) {
		pChild->PostUpdate();
	}
}

void WaterGridObject::OnTraceHit(const RayTraceHitResult& hitResult)
{
}

void WaterGridObject::CreateNoiseTexture()
{
	if (m_NoiseTexture.IsValid()) {
		return;
	}

	constexpr uint32 unNoiseSize = 128;

	std::uniform_real_distribution<float> uid{ 0.0f, 1.0f };
	std::vector<Vector4> noiseData;
	noiseData.reserve(static_cast<size_t>(unNoiseSize) * unNoiseSize);

	for (uint32 i = 0; i < unNoiseSize * unNoiseSize; ++i) {
		auto& rng = RandomGenerator::g_dre;
		noiseData.emplace_back(uid(rng), uid(rng), uid(rng), 1.0f);
	}

	m_NoiseTexture = TEXTURE->LoadTextureFromRawData(
		"WaterNoise",
		std::move(noiseData),
		unNoiseSize,
		unNoiseSize,
		DXGI_FORMAT_R32G32B32A32_FLOAT);
}
