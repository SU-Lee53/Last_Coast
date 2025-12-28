#pragma once
#include "Sprite.h"

#define MAX_UI_LAYER_COUNT 3
#define MAX_SPRITE_PER_DRAW 100

struct SpriteParameter {
	SpriteRect spriteRect;	// 그림 / 텍스트가 들어갈 사각형 (Window 좌표계)
	bool bClickable;
	SPRITE_TYPE eSpriteType;
};

using UILayerContainerType = std::array<std::array<std::vector<std::shared_ptr<Sprite>>, SPRITE_TYPE_COUNT>, MAX_UI_LAYER_COUNT>;

class UILayer {
public:
	void Add(std::shared_ptr<Sprite> pSprite, UINT nSpriteType, UINT nLayerIndex) {
		///assert(nLayer < 3);
		m_UILayers[nLayerIndex][nSpriteType].push_back(pSprite);
	}

	void Clear() {
		for (auto& layer : m_UILayers) {
			for (auto& sprites : layer) {
				sprites.clear();
			}
		}
	}

	UILayerContainerType& GetLayers() { return m_UILayers; }

private:
	UILayerContainerType m_UILayers;
};

class UIManager {
public:
	UIManager(ComPtr<ID3D12Device> pd3dDevice);

	void Add(std::shared_ptr<Sprite> pSprite, UINT nSpriteType, UINT nLayerIndex);
	void Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList);
	void Clear();

	void CheckButtonClicked(POINT ptClickedPos);

private:
	void CreateRootSignature();
	void CreatePipelineState();

private:
	ComPtr<ID3D12RootSignature> m_pd3dUIRootSignature = nullptr;
	ComPtr<ID3D12PipelineState> m_pd3dUIPipelineStates[3];

	ComPtr<ID3D12Device>			m_pd3dDevice = nullptr;	// GameFramewok::m_pd3dDevice 의 참조
	ComPtr<ID3D12DescriptorHeap>	m_pd3dDescriptorHeap = nullptr;

	UILayer m_UILayer{};
	std::vector<std::shared_ptr<Sprite>> m_pBillboardsInWorld;

};

