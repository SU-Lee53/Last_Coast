#include "pch.h"
#include "ParticleManager.h"

void ParticleManager::Initialize()
{
	m_pActiveEffects.Clear();
	m_RenderBatches.clear();

	RegisterParticleTextures();
	m_TextureCache.PreloadAll();
}

void ParticleManager::Update()
{
	m_pActiveEffects.ForEachAlive([](const std::shared_ptr<IParticleEffect>& pEffect) {
		pEffect->Update();
	});
	
	m_pActiveEffects.RemoveAndResetIfAlive([](const std::shared_ptr<IParticleEffect>& pEffect) {
		return pEffect->IsDead();
	});

	BuildRenderBatches();
}

void ParticleManager::BuildRenderBatches()
{
	m_RenderBatches.clear();

	m_pActiveEffects.ForEachAlive([this](const std::shared_ptr<IParticleEffect>& pEffect) {
		pEffect->CollectRenderData(m_RenderBatches);
	});

	//	const Vector3& v3CameraPos = CUR_SCENE->GetCamera()->GetPosition();
	//	for (auto& batch : m_RenderBatches) {
	//		if (batch.eSortMode != PARTICLE_SORT_MODE::BACK_TO_FRONT) {
	//			continue;
	//		}
	//		std::sort(batch.drawDatas.begin(), batch.drawDatas.end(), [&v3CameraPos](const ParticleDrawData& lhs, const ParticleDrawData& rhs) {
	//			const float dl = Vector3::DistanceSquared(lhs.v3Position, v3CameraPos);
	//			const float dr = Vector3::DistanceSquared(rhs.v3Position, v3CameraPos);
	//			return dl > dr;
	//			});
	//	}

}

void ParticleManager::RegisterParticleTextures()
{
	m_TextureCache.Register(
		PARTICLE_TEXTURE_ID::MUZZLE_FLASH_CORE,
		"../Resources/Particles/MuzzleFlash_Core.png"
	);

	m_TextureCache.Register(
		PARTICLE_TEXTURE_ID::MUZZLE_FLASH_SHAPE,
		"../Resources/Particles/MuzzleFlash_Shape.png"
	);

	m_TextureCache.Register(
		PARTICLE_TEXTURE_ID::BLOOD_MIST,
		"../Resources/Particles/Blood_Mist.png"
	);

	m_TextureCache.Register(
		PARTICLE_TEXTURE_ID::BLOOD_DROP,
		"../Resources/Particles/Blood_Drop.png"
	);

	m_TextureCache.Register(
		PARTICLE_TEXTURE_ID::BLOOD_SPLATTER_SMALL,
		"../Resources/Particles/Blood_Splatter_Small.png"
	);

	m_TextureCache.Register(
		PARTICLE_TEXTURE_ID::SMOKE_PUFF,
		"../Resources/Particles/Smoke_Puff.png"
	);

	m_TextureCache.Register(
		PARTICLE_TEXTURE_ID::IMPACT_DUST,
		"../Resources/Particles/Impact_Dust.png"
	);
}
