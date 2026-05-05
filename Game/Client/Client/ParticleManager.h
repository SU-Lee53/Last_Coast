#pragma once
#include "ParticleEffect.h"
#include "ParticleRenderBatch.h"
#include "ParticleTextureCache.h"

class ParticleManager {

	DECLARE_SINGLE(ParticleManager)

public:
	void Initialize();
	void Update();
	
	template<typename T, typename... Args> requires std::derived_from<T, IParticleEffect>
	std::shared_ptr<T> Spawn(const ParticleEffectSpawnDesc& desc, Args&&... args);

	const std::vector<ParticleRenderBatch>& GetRenderBatches() const { return m_RenderBatches; }

	TextureRef<Texture> GetTextureCached(PARTICLE_TEXTURE_ID eTextureID) { return m_TextureCache.Get(eTextureID); }

private:
	void BuildRenderBatches();
	void RegisterParticleTextures();

private:
	TypedObjectPool<IParticleEffect> m_pActiveEffects;
	std::vector<ParticleRenderBatch> m_RenderBatches;

	ParticleTextureCache m_TextureCache;

};

template<typename T, typename... Args> requires std::derived_from<T, IParticleEffect>
std::shared_ptr<T> ParticleManager::Spawn(const ParticleEffectSpawnDesc& desc, Args&&... args)
{
	auto pEffect = std::make_shared<T>(std::forward<Args>(args)...);

	pEffect->Initialize();
	pEffect->Play(desc);
	m_pActiveEffects.Add(pEffect);

	return pEffect;
}
