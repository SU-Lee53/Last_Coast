#pragma once
#include "ParticleTypes.h"


class ParticleTextureCache {
public:
	void Register(PARTICLE_TEXTURE_ID eID, std::string strPath) {
		m_TexturePaths[ToIndex(eID)] = std::move(strPath);
	}

	TextureRef<Texture> Get(PARTICLE_TEXTURE_ID eID) {
		const size_t idx = ToIndex(eID);

		if (!m_TextureRefs[idx].IsValid()) {
			const std::string& path = m_TexturePaths[idx];

			if (path.empty()) {
				__debugbreak();
				return {};
			}

			m_TextureRefs[idx] = TEXTURE->LoadTexture(path);
		}

		return m_TextureRefs[idx];
	}

	void PreloadAll() {
		for (size_t i = 0; i < m_TexturePaths.size(); ++i) {
			if (!m_TexturePaths[i].empty() && !m_TextureRefs[i].IsValid()) {
				m_TextureRefs[i] = TEXTURE->LoadTexture(m_TexturePaths[i]);
			}
		}
	}

	void Clear() {
		for (auto& ref : m_TextureRefs) {
			ref = {};
		}
	}

private:
	static constexpr size_t ToIndex(PARTICLE_TEXTURE_ID eID) {
		return static_cast<size_t>(eID);
	}

private:
	static constexpr size_t g_nTextureCount =
		static_cast<size_t>(PARTICLE_TEXTURE_ID::COUNT);

	std::array<std::string, g_nTextureCount> m_TexturePaths{};
	std::array<TextureRef<Texture>, g_nTextureCount> m_TextureRefs{};
};
