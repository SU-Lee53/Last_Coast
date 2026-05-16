#pragma once

struct BloomParameters
{
	float fThreshold = 1.0f;
	float fSoftKnee = 0.5f;
	float fIntensity = 0.6f;

	float fRadius = 1.0f;
	Vector3 pad;
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

class PostProcessingVolume
{
public:
	void Update();
	void ShowDebugOptions();

	CB_BLOOM_DATA GetBloomCBData(XMINT2 xmi2InputSize, XMINT2 xmi2OutputSize) const;

	const BloomParameters& GetBloomParameters() const { return m_Bloom; }

private:
	BloomParameters m_Bloom;

};

