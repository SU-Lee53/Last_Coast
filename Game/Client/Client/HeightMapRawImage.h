#pragma once

class HeightMapRawImage {
public:
	HRESULT LoadFromFile(
		const std::string& strFilename,
		uint32 unWidth,
		uint32 unHeight,
		float fOriginX,
		float fOriginZ,
		float fScaleX,			// cm per quad (Scale.x from unreal)
		float fScaleZ,			// cm per quad (Scale.y from unreal)
		float fHeightScale);	// ZScale

	// Local
	float GetHeightLocal(float fX, float fZ) const;
	Vector3 GetNormalLocal(float fX, float fZ) const;
	Vector3 GetTangentLocal(float fX, float fZ) const;

	// World
	float GetHeightWorld(float fWorldX, float fWorldZ) const;
	Vector3 GetNormalWorld(float fWorldX, float fWorldZ) const;
	Vector3 GetTangentWorld(float fWorldX, float fWorldZ) const;

private:
	float SampleHeight(uint32 x, uint32 z) const;

private:
	// RAW data
	std::vector<uint16> m_HeightMapRawData;

	// Resolution (NumQuadX + 1) * (NumQuadZ + 1)
	uint32 m_unWidth = 0;
	uint32 m_unHeight = 0;
	float m_fOriginX = 0;
	float m_fOriginZ = 0;
	
	Vector3 m_v3Scale = Vector3{ 0,0,0 };	// cm

};

