#pragma once

struct BloomParameters {
	float fThreshold = 1.0f;
	float fSoftKnee = 0.5f;
	float fIntensity = 0.6f;

	float fRadius = 1.0f;
	Vector3 pad;
};

struct SSAOParameters {
	float	fRadius = 3.0f;
	float	fBias = 0.001f;
	float	fPower = 1.5f;
	float	fIntensity = 1.0f;

	int		nSampleCount = 16;
	float	fDepthSigma = 120.0f;
	float	fNormalSigma = 16.0f;
	float	fNoiseScale = 1.0f;
};

struct ScreenFXParameters {
	float fGrainStrength = 0.015f;
	float fGrainScale = 1.0f;
	float fVignetteStrength = 0.25f;
	float fVignetteRadius = 0.75f;
	float fVignetteSoftness = 0.45f;
};

struct LightShaftParameters {
	bool bEnable = true;
	float fIntensity = 0.65f;
	float fDecay = 0.94f;
	float fDensity = 0.85f;
	float fWeight = 0.08f;
	float fExposure = 1.0f;
	float fDepthThreshold = 0.999f;
	int nSampleCount = 48;
};

struct FogParameters {
	Vector4 v4FogColor = Vector4(0.62f, 0.68f, 0.72f, 1.0f);

	float fFogStartDistance = 5.0f;
	float fFogCutOffDistance = 0.0f;
	float fFogDistanceDensity = 0.0035f;
	float fFogDistancePower = 1.0f;

	float fFogHeightDensity = 0.02f;
	float fFogHeightFalloff = 0.06f;
	float fFogBaseHeightOffset = 0.0f;
	float fFogHeightStartDistance = 0.0f;

	float fFogMaxOpacity = 0.0f;
	float fFogDetailStrength = 0.0f;
	float fFogDetailNoiseScale = 0.0005f;
	float fFogDetailNoiseSpeed = 0.02f;

	float fFogDetailHeightRange = 300.0f;
	float fFogDetailDistanceStart = 500.0f;
	float fFogDetailDirectionalScattering = 0.0f;
	float pad = 0.0f;
};

struct CB_BLOOM_DATA {
	float gBloomThreshold;
	float gBloomSoftKnee;
	float gBloomIntensity;
	float gBloomRadius;

	XMINT2 gInputSize;
	XMINT2 gOutputSize;
};

struct CB_SSAO_DATA {
	float	gfRadius;
	float	gfBias;
	float	gfPower;
	float	gfIntensity;

	int		gnSampleCount;
	float	gfDepthSigma;
	float	gfNormalSigma;
	float	gfNoiseScale;
};

struct CB_SCREEN_FX_DATA {
	float gGrainStrength;
	float gGrainScale;
	float gVignetteStrength;
	float gVignetteRadius;

	float gfVignetteSoftness;
	Vector2 pad;
};

struct CB_LIGHT_SHAFT_DATA {
	Vector2 gv2LightScreenPosition;
	float gfIntensity;
	float gfDecay;

	float gfDensity;
	float gfWeight;
	float gfExposure;
	float gfDepthThreshold;

	int gnSampleCount;
	int gnEnable;
	Vector2 pad;
};

struct CB_FOG_DATA
{
	Vector4 gfogColor;

	float gfFogStartDistance;
	float gfFogCutOffDistance;
	float gfFogDistanceDensity;
	float gFogDistancePower;

	float gfFogHeightDensity;
	float gfFogHeightFalloff;
	float gfFogBaseHeight;
	float gfFogHeightStartDistance;

	float gfFogMaxOpacity;
	float gfFogDetailStrength;
	float gfFogDetailNoiseScale;
	float gfFogDetailNoiseSpeed;

	float gfFogDetailHeightRange;
	float gfFogDetailDistanceStart;
	float gfFogDetailDirectionalScattering;
	float pad;
};

struct PostProcessingParameters {
	BloomParameters Bloom;
	SSAOParameters SSAO;
	ScreenFXParameters ScreenFX;
	LightShaftParameters LightShaft;
	FogParameters Fog;

	friend std::ifstream& operator>>(std::ifstream& in, PostProcessingParameters& params) {
		in.read((char*)(&params), sizeof(PostProcessingParameters));
		return in;
	}

	friend std::ofstream& operator<<(std::ofstream& out, const PostProcessingParameters& params) {
		out.write((char*)(&params), sizeof(PostProcessingParameters));
		return out;
	}
};

class PostProcessingVolume {
public:
	void Update();
	void ShowDebugOptions();

	bool SaveParametersToBinary(const std::string& strSaveName) const;
	bool LoadFromFiles(const std::string& strSaveName);

	CB_BLOOM_DATA GetBloomCBData(XMINT2 xmi2InputSize, XMINT2 xmi2OutputSize) const;
	CB_SSAO_DATA GetSSAOCBData() const;
	CB_SCREEN_FX_DATA GetScreenFXCBData() const;
	CB_LIGHT_SHAFT_DATA GetLightShaftCBData(const Vector2& v2LightScreenPosition) const;
	CB_FOG_DATA GetFogCBData() const;

	const BloomParameters& GetBloomParameters() const { return m_Parameters.Bloom; }
	const SSAOParameters& GetSSAOParameters() const { return m_Parameters.SSAO; }
	const ScreenFXParameters& GetScreenFXParameters() const { return m_Parameters.ScreenFX; }
	const LightShaftParameters& GetLightShaftParameters() const { return m_Parameters.LightShaft; }
	const FogParameters& GetFogParameters() const { return m_Parameters.Fog; }

	// 게임 이벤트 런타임 조정용 (쓰기 가능). 값은 매 프레임 라이브로 반영됨.
	BloomParameters& GetBloomParameters() { return m_Parameters.Bloom; }
	SSAOParameters& GetSSAOParameters() { return m_Parameters.SSAO; }
	ScreenFXParameters& GetScreenFXParameters() { return m_Parameters.ScreenFX; }
	LightShaftParameters& GetLightShaftParameters() { return m_Parameters.LightShaft; }
	FogParameters& GetFogParameters() { return m_Parameters.Fog; }

private:
	PostProcessingParameters m_Parameters;

	std::string m_strSaveName;

private:
	TextureRef<Texture> m_NoiseTexture;

	inline const static std::string g_strSavePath = "../Resources/Scenes/PostProcessingVolume_";

};
