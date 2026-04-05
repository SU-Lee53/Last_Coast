#pragma once
#include "RenderPass.h"

template<typename KeyType, typename ElemType>
class IndexMap {
private:
	std::unordered_map<KeyType, size_t> m_IndexMap;
	std::vector<ElemType> m_Elements;

public:
	void Reserve(size_t newSize) {
		m_IndexMap.reserve(newSize);
		m_Elements.reserve(newSize);
	}
	
	std::pair<size_t, bool> Insert(const KeyType& k, const ElemType& v) {
		auto [idx, bResult] = m_IndexMap.emplace(k, m_Elements.size());
		if (bResult) {
			m_Elements.push_back(v);
		}

		return { idx->second, bResult };
	}

	void Clear() {
		m_IndexMap.clear();
		m_Elements.clear();
	}

	size_t Size() { return m_Elements.size(); }

	//ElemType& operator[](KeyType key) { return m_Elements[m_IndexMap[key]]; }
	ElemType& operator[](size_t idx) { return m_Elements[idx]; }

	const std::vector<ElemType>& GetElements() { return m_Elements; }
};


class GBufferPass : public IRenderPass {
	struct RenderParameter {
		CB_INSTANCE_DATA cbInstanceData;
		std::vector<WorldTransformData> sbWorldTransformData;
		std::vector<AnimationController*> pAnimationControllers;
	};

	using RenderQueue = std::vector<std::pair<IMesh*, GBufferPass::RenderParameter>>;

public:
	virtual void Initialize() override;

	virtual void Render(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		const RenderPassInput& input,
		OUT RenderPassOutput& output,
		OUT DescriptorHandle& outDescHandle) const override;

	virtual void OnPreRender(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		const RenderPassInput& input,
		OUT RenderPassOutput& output, 
		OUT DescriptorHandle& outDescHandle) const override;


	virtual void OnPostRender(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		const RenderPassInput& input, 
		OUT RenderPassOutput& output, 
		OUT DescriptorHandle& outDescHandle) const override;

	virtual void ShowDebugInfo() override;

private:
	void SetRenderTargets(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList) const;

	void BindGeometryData(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		const std::vector<std::shared_ptr<IGameObject>>& frustumCulled, 
		OUT DescriptorHandle& outDescHandle) const;
	
	void BindTerrainData(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		OUT DescriptorHandle& outDescHandle) const;

	void DrawGeometry(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, 
		OUT DescriptorHandle& outDescHandle) const;
	
	void DrawTerrain(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, 
		OUT DescriptorHandle& outDescHandle) const;

private:
	mutable RenderQueue m_RenderQueueCached;
	mutable uint32 m_unFrustumCulled = 0;

	mutable struct CachedData {
		IndexMap<MeshRenderer::ID, std::pair<const MeshRenderer*, std::vector<const IGameObject*>>> m_FrustumCulledMap;
		IndexMap<IMaterial::ID, MaterialData> m_MaterialMap;
		IndexMap<Texture::ID, const TextureRef<Texture>*> m_TextureMap;

		void Clear() {
			m_FrustumCulledMap.Clear();
			m_MaterialMap.Clear();
			m_TextureMap.Clear();
		}

	} m_CachedData;


};
