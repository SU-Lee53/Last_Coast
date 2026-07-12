#pragma once
#include "ParticleEffect.h"

// 헬기 추락 잔해 대형 화재 — FireEffect(차량 화재)와 동일 구성, 수치만 대형(약 2.5배).
// 파티클 풀이 타입 단위 재사용이라 스케일 파라미터 대신 별도 타입으로 분리.
class CrashFireEffect : public IParticleEffect {
public:
	virtual void Initialize() override;

private:
	void CreateFlameEmitter();
	void CreateHeatCoreEmitter();
	void CreateSmokeEmitter();
};
