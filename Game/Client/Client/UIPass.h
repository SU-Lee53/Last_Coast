#pragma once
#include "RenderPass.h"
class UIPass : public IRenderPass {
public:
	virtual void Initialize() override;

	virtual void Render(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		const RenderPassInput& input,
		OUT RenderPassOutput& output,
		OUT DescriptorHandle& outDescHandle) override;

	virtual void OnPreRender(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		const RenderPassInput& input,
		OUT RenderPassOutput& output,
		OUT DescriptorHandle& outDescHandle) override;


	virtual void OnPostRender(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		const RenderPassInput& input,
		OUT RenderPassOutput& output,
		OUT DescriptorHandle& outDescHandle) override;

private:
	void CreatePipelineState();

private:
	ComPtr<ID3D12PipelineState> m_pd3dTextPipelineState;
	ComPtr<ID3D12PipelineState> m_pd3dSpritePipelineState;

private:
	// Cache
	struct CachedData {
		std::array<std::vector<UIRectData>, UIBoard::g_unUILayers> textDatas;
		std::array<std::vector<UIRectData>, UIBoard::g_unUILayers> spriteDatas;

		IndexMap<Texture::ID, const TextureRef<Texture>*> textureMap;

		void Clear() {
			for (int i = 0; i < UIBoard::g_unUILayers; ++i) {
				textDatas[i].clear();
				spriteDatas[i].clear();
			}

			textureMap.Clear();
		}

	} m_CachedData;

};

