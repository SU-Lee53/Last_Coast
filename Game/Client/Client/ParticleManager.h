#pragma once
#include "ParticleEffect.h"
#include "ParticleRenderBatch.h"
#include "ParticleTextureCache.h"

class ParticleManager {

	DECLARE_SINGLE(ParticleManager)

public:
	void Initialize();
	void Shutdown();
	void Update();
	
	template<typename T, typename... Args> requires std::derived_from<T, IParticleEffect>
	std::shared_ptr<T> Spawn(const ParticleEffectSpawnDesc& desc, Args&&... args);

	const std::vector<ParticleRenderBatch>& GetRenderBatches() const { return m_RenderBatches; }

	TextureRef<Texture> GetTextureCached(PARTICLE_TEXTURE_ID eTextureID) { return m_TextureCache.Get(eTextureID); }

private:
	void BuildRenderBatches();
	void RegisterParticleTextures();
	void ReleaseDeadEffects();

	template<typename T, typename... Args> requires std::derived_from<T, IParticleEffect>
	std::shared_ptr<T> AcquireEffect(Args&&... args);

private:
	TypedObjectPool<IParticleEffect> m_pActiveEffects;
	std::unordered_map<std::type_index, std::vector<std::shared_ptr<IParticleEffect>>> m_pEffectPools;
	std::vector<std::shared_ptr<IParticleEffect>> m_pDeadEffectsScratch;
	std::vector<ParticleRenderBatch> m_RenderBatches;

	ParticleTextureCache m_TextureCache;

};

template<typename T, typename... Args> requires std::derived_from<T, IParticleEffect>
std::shared_ptr<T> ParticleManager::Spawn(const ParticleEffectSpawnDesc& desc, Args&&... args)
{
	auto pEffect = AcquireEffect<T>(std::forward<Args>(args)...);

	pEffect->Play(desc);
	m_pActiveEffects.Add(pEffect);

	return pEffect;
}

template<typename T, typename... Args> requires std::derived_from<T, IParticleEffect>
std::shared_ptr<T> ParticleManager::AcquireEffect(Args&&... args)
{
	auto& pPool = m_pEffectPools[std::type_index(typeid(T))];
	if (!pPool.empty()) {
		std::shared_ptr<IParticleEffect> pEffect = pPool.back();
		pPool.pop_back();
		return std::static_pointer_cast<T>(pEffect);
	}

	auto pEffect = std::make_shared<T>(std::forward<Args>(args)...);
	pEffect->Initialize();
	return pEffect;
}
