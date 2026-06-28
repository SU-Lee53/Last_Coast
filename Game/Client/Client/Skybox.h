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
	bool SaveSkyboxParameters() const;
	bool LoadSkyboxParameters();
	bool SaveSkyboxParameters(const std::string& strSaveName) const;
	bool LoadSkyboxParameters(const std::string& strSaveName);

	void SetDayNightBlend(float fTime) { m_fTimeOfDayHours = std::clamp(fTime, 0.0f, 1.0f) * 24.0f; }
	void SetTimeOfDayHours(float fHours) { m_fTimeOfDayHours = NormalizeTimeOfDayHours(fHours); }

	// 현재 시간(0~1, SetDayNightBlend 와 동일 스케일). 환경 프리셋 페이드 시작값 캡처용.
	float GetDayNightBlend() const { return m_fTimeOfDayHours / 24.0f; }

	// 하늘에서 태양 쪽을 가리키는 단위벡터(시간에 따라 갱신됨). 해질녘 카메라 연출용.
	const Vector3& GetSunDirection() const { return m_v3SunDirection; }

private:
	void SetDefaultSkyboxParameters();
	void UpdateTimeDerivedValues();
	void SyncSunLightWithScene() const;

	static std::string MakeSkyboxParametersPath(const std::string& strSaveName);
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
	inline const static  std::string g_strSavePath = "../Resources/Skybox/Skybox_";
	std::string m_strSaveName = "Parameters";
};
