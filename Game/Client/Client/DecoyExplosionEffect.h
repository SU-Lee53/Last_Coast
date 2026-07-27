#pragma once
#include "ParticleEffect.h"

// 디코이 수류탄 전용 폭발 — 데미지 없는 유인용이라 화염 대신 폭죽 컨셉.
// 기존 파티클 텍스처(MuzzleFlash_Core/Smoke_Puff)를 색만 바꿔 재활용:
// 흰 섬광 + 6색 불꽃 스파크 + 파스텔 연막 + 잔반짝임.
class DecoyExplosionEffect : public IParticleEffect {
public:
	virtual void Initialize() override;
	virtual void Play(const ParticleEffectSpawnDesc& desc) override;

private:
	void CreateFlashEmitter();
	void CreateSparkEmitter(const char* name, const Vector4& colorStart, const Vector4& colorEnd);
	void CreateTintedSmokeEmitter(const char* name, const Vector4& color);
	void CreateTwinkleEmitter(const char* name, const Vector4& colorStart, const Vector4& colorEnd);

private:
	std::shared_ptr<Sound> m_pSound = nullptr;

};
