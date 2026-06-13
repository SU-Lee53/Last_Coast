#pragma once

class IComputePass;

class Skybox {
public:
	struct CubeMapIDPair {
		CubeMapIDPair() = default;
		CubeMapIDPair(std::pair<Texture::ID, Texture::ID> IDPair) : srvID{ IDPair.first }, uavID{ IDPair.second } {}
		Texture::ID srvID;
		Texture::ID uavID;
	};

public:
	void Initialize();
	void Update();

	Texture::ID GetDaySkyBoxTextureID() const { return m_CubeMapDay.srvID; }
	Texture::ID GetNightSkyBoxTextureID() const { return m_CubeMapNight.srvID; }

	SkyboxData MakeCBData() const;
	void ShowControllImGui();

	void SetDayNightBlend(float fTime) { m_fTimeOfDayHours = std::clamp(fTime, 0.0f, 1.0f) * 24.0f; }
	void SetTimeOfDayHours(float fHours) { m_fTimeOfDayHours = NormalizeTimeOfDayHours(fHours); }

private:
	void SaveParametersToJson() const;
	void UpdateTimeDerivedValues();
	void SyncSunLightWithScene() const;

	static float NormalizeTimeOfDayHours(float fHours);
	static float GetEffectiveTwilightWidth(float fWidth);

private:
	CubeMapIDPair m_CubeMapDay;
	CubeMapIDPair m_CubeMapNight;

private:
	Vector3 m_v3NoonHorizontalDirection;
	
	// CB parameters
	float m_fDayNightBlend;		// Main arameter for control skybox
	float m_fTimeOfDayHours = 12.0f;
	Vector3 m_v3SunDirection;	// -v3SunDirection = v3MoonDirection

	float	m_fSunIntensity;
	float	m_fMoonIntensity;
	float	m_fDirectionalLightIntensityScale = 0.04f;
	float	m_fAmbientIntensity = 0.08f;
	float	m_fSunDiskSize;
	float	m_fMoonDiskSize;

	float	m_fSunGlowSize;
	float	m_fMoonGlowSize;
	float	m_fTwilightWidth;
	float	m_fTwilightIntensity;

	float	m_fTwilightSunFocus; 
	float	m_fCloudCoverage;
	float	m_fCloudDensity;
	float	m_fCloudSpeed;

	float	 m_fCloudScale;
	float	 m_fCloudLightIntensity;
	float	 m_fStarDensity;
	float	 m_fStarScale;

	float	m_fSkyIntensity;
	Vector3 m_v3TwilightColor;
	Vector3 m_v3SunColor;
	Vector3 m_v3MoonColor;
	Vector3 m_v3DayZenithColor;
	Vector3 m_v3DayHorizonColor;
	Vector3 m_v3NightZenithColor;
	Vector3 m_v3NightHorizonColor;

	std::shared_ptr<IComputePass> m_pPass;
	inline const static  std::string g_strSkyboxBasePath = "../Resources/Skybox/";
};

