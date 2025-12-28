#pragma once
#include "Effect.h"

using EffectPair = std::pair<std::shared_ptr<Effect>, std::vector<EffectParameter>>;

class EffectManager {
public:
	EffectManager() {}

public:
	void Initialize(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList);
	void Update(float fTimeElapsed);
	void Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList);

	template<typename T> requires std::derived_from<T, Effect>
	void AddEffect(const EffectParameter& effectParameter);

private:
	void CreateRootSignature();

private:
	ComPtr<ID3D12Device> m_pd3dDevice;	// Ref to GameFramework::m_pd3dDevice;
	ComPtr<ID3D12RootSignature> m_pd3dRootSignature;

	UINT m_nParticles = 0;;
	std::unordered_map<std::type_index, EffectPair> m_pEffects;

};

template<typename T> requires std::derived_from<T, Effect>
void EffectManager::AddEffect(const EffectParameter& effectParameter)
{
	if (m_nParticles >= MAX_EFFECT_PER_DRAW) {
		return;
	}

	m_pEffects[typeid(T)].second.push_back(effectParameter);
	m_nParticles++;
}

