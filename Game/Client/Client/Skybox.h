#pragma once

class IComputePass;

class Skybox {
public:
	void Initialize(const std::string& strDay);

	Texture::ID GetDaySkyBoxTextureID() const { return m_SkyboxSRVID; }
	Texture::ID GetNightSkyBoxTextureID() const { return m_SkyboxSRVID; }

private:
	Texture::ID m_SkyboxSRVID;
	Texture::ID m_SkyboxUAVID;

	std::shared_ptr<IComputePass> m_pPass;

	const std::string g_strTextureBasePath = "../Resources/Skybox/";
};

