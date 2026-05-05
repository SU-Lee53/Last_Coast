#include "pch.h"
#include "ParticleEffect.h"

void IParticleEffect::Update()
{
	if (!m_bPlaying || m_bDead) {
		return;
	}

	bool bAnyAlive = false;
	for (auto& pEmitter : m_pEmitters) {
		pEmitter->Update(m_SpawnDesc);
		if (!pEmitter->IsDead()) {
			bAnyAlive = true;
		}
	}

	if (!bAnyAlive) {
		m_bDead = true;
	}

}

void IParticleEffect::Play(const ParticleEffectSpawnDesc& desc)
{
	m_SpawnDesc = desc;
	m_bPlaying = true;
	m_bDead = false;
}

void IParticleEffect::CollectRenderData(std::vector<ParticleRenderBatch>& outRenderBatches) const
{
	for (const auto& pEmitter : m_pEmitters) {
		pEmitter->CollectRenderData(outRenderBatches, m_SpawnDesc);
	}
}
