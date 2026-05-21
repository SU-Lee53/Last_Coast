#pragma once

struct BloomParameters
{
	float fThreshold = 1.0f;
	float fSoftKnee = 0.5f;
	float fIntensity = 0.6f;

	float fRadius = 1.0f;
	Vector3 pad;
};

struct SSAOParameters
{
	float	fRadius = 3.0f;
	float	fBias = 0.001f;
	float	fPower = 1.5f;
	float	fIntensity = 1.0f;
	int		nSampleCount = 16;
	float	fDepthSigma = 120.0f;
	float	fNormalSigma = 16.0f;
	float	fNoiseScale = 1.0f;
};

struct CB_BLOOM_DATA
{
	float gBloomThreshold;
	float gBloomSoftKnee;
	float gBloomIntensity;
	float gBloomRadius;

	XMINT2 gInputSize;
	XMINT2 gOutputSize;
};

struct CB_SSAO_DATA
{
	float	gfRadius;
	float	gfBias;
	float	gfPower;
	float	gfIntensity;

	int		gnSampleCount;
	float	gfDepthSigma;
	float	gfNormalSigma;
	float	gfNoiseScale;
};

class PostProcessingVolume
{
public:
	void Update();
	void ShowDebugOptions();

	CB_BLOOM_DATA GetBloomCBData(XMINT2 xmi2InputSize, XMINT2 xmi2OutputSize) const;
	CB_SSAO_DATA GetSSAOCBData() const;

	const BloomParameters& GetBloomParameters() const { return m_Bloom; }
	const SSAOParameters& GetSSAOParameters() const { return m_SSAO; }

private:
	BloomParameters m_Bloom;
	SSAOParameters m_SSAO;

private:
	TextureRef<Texture> m_NoiseTexture;

};

