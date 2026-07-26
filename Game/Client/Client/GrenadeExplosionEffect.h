#pragma once
#include "ParticleEffect.h"

// 수류탄 전용 소형 폭발 — ExplosionEffect(헬기 추락급)의 축소판.
// 시각 스케일은 반경 500cm 기준으로 튜닝 — 실제 타격 반경(EXPLODE_RADIUS = 700cm)보다
// 의도적으로 작게 유지 중. 반경/연출 스케일을 바꾸려면 InitSizeRandomModule 값들을 조정할 것.
class GrenadeExplosionEffect : public IParticleEffect {
public:
	virtual void Initialize() override;
	virtual void Play(const ParticleEffectSpawnDesc& desc) override;

private:
	void CreateFlashEmitter();
	void CreateFireballEmitter();
	void CreateSmokeEmitter();
	void CreateDustEmitter();

private:
	std::shared_ptr<Sound> m_pSound = nullptr;

};
