#pragma once
#include "StaticObject.h"
#include "Texture.h"
class WaterGridObject : public StaticObject {
public:
	virtual void Initialize() override;
	virtual void Update() override;
	virtual void PostUpdate() override;

	virtual void OnTraceHit(const RayTraceHitResult& hitResult) override;

private:
	void CreateNoiseTexture();

private:
	TextureRef<Texture> m_NoiseTexture;

};

