#include "pch.h"
#include "HeightMapRawImage.h"

HRESULT HeightMapRawImage::LoadFromFile(const std::string& strFilename, uint32 unWidth, uint32 unHeight, float fOriginX, float fOriginZ, float fScaleX, float fScaleZ, float fHeightScale)
{
	m_unWidth = unWidth;
	m_unHeight = unHeight;
	m_fOriginX = fOriginX;
	m_fOriginZ = fOriginZ;
	m_v3Scale = Vector3{ fScaleX, fHeightScale, fScaleZ };

	std::ifstream in{ strFilename, std::ios::binary };
	if (!in) {
		return E_INVALIDARG;
	}
	
	m_HeightMapRawData.resize(unWidth * unHeight);
	in.read(reinterpret_cast<char*>(m_HeightMapRawData.data()), m_HeightMapRawData.size() * sizeof(uint16));

	if (!in) {
		return E_FAIL;
	}

	return S_OK;
}

float HeightMapRawImage::GetHeightLocal(float fX, float fZ) const
{
	const float fGridX = fX / m_v3Scale.x;
	const float fGridZ = fZ / m_v3Scale.z;

	int32 x0 = static_cast<int32>(floorf(fGridX));
	int32 z0 = static_cast<int32>(floorf(fGridZ));
	int32 x1 = x0 + 1;
	int32 z1 = z0 + 1;

	x0 = std::clamp(x0, 0, static_cast<int32>(m_unWidth) - 1);
	x1 = std::clamp(x1, 0, static_cast<int32>(m_unWidth) - 1);
	z0 = std::clamp(z0, 0, static_cast<int32>(m_unHeight) - 1);
	z1 = std::clamp(z1, 0, static_cast<int32>(m_unHeight) - 1);

	float tx = fGridX - x0;
	float tz = fGridZ - z0;

	/*
		0.0        tx       1.0
		|----------|--------|
		h01               h11
		+---------(h1)------+    --- 0.0
		|                   |     |
		|  lerp(h0, h1, tz) |     |
		|          +--------+fZ   + tz
		|          |        |     |
		|          |        |     |
		|          |        |     |
		+---------(h0)------+    --- 1.0
		h00		   fX	    h01
		|----------|--------|
		0.0        tx       1.0
	*/

	float h00 = SampleHeight(x0, z0);
	float h10 = SampleHeight(x1, z0);
	float h01 = SampleHeight(x0, z1);
	float h11 = SampleHeight(x1, z1);

	float h0 = std::lerp(h00, h10, tx);
	float h1 = std::lerp(h01, h11, tx);

	return std::lerp(h0, h1, tz);
}

Vector3 HeightMapRawImage::GetNormalLocal(float fX, float fZ) const
{
	const float fEpsilonX = m_v3Scale.x;
	const float fEpsilonZ = m_v3Scale.z;

	float fLeft = GetHeightLocal(fX - fEpsilonX, fZ);
	float fRight = GetHeightLocal(fX + fEpsilonX, fZ);
	float fBottom = GetHeightLocal(fX, fZ - fEpsilonZ);
	float fTop = GetHeightLocal(fX, fZ + fEpsilonZ);

	Vector3 v3DeltaX = { 2.0f * fEpsilonX, fRight - fLeft, 0.f };
	Vector3 v3DeltaZ = { 0.f, fTop - fBottom, 2.0f * fEpsilonZ };

	Vector3 v3Normal = v3DeltaZ.Cross(v3DeltaX);
	v3Normal.Normalize();
	return v3Normal;
}

Vector3 HeightMapRawImage::GetTangentLocal(float fX, float fZ) const
{
	const float fEpsilonX = m_v3Scale.x;

	float fLeft = GetHeightLocal(fX - fEpsilonX, fZ);
	float fRight = GetHeightLocal(fX + fEpsilonX, fZ);

	Vector3 v3Tangent = { 2.0f * fEpsilonX, fRight - fLeft, 0.f };
	v3Tangent.Normalize();
	return v3Tangent;
}

bool HeightMapRawImage::IsInsideLocal(float fX, float fZ)
{
	const float fGridX = fX / m_v3Scale.x;
	const float fGridZ = fZ / m_v3Scale.z;

	return
		fGridX >= 0.f &&
		fGridZ >= 0.f &&
		fGridX <= (float)(m_unWidth - 1) &&
		fGridZ <= (float)(m_unHeight - 1);
}

float HeightMapRawImage::GetHeightWorld(float fWorldX, float fWorldZ) const
{
	const float ueX = fWorldZ;
	const float ueZ = fWorldX;

	const float fLocalX = ueX - m_fOriginX;
	const float fLocalZ = ueZ - m_fOriginZ;
	return GetHeightLocal(fLocalX, fLocalZ);
}

Vector3 HeightMapRawImage::GetNormalWorld(float fWorldX, float fWorldZ) const
{
	const float ueX = fWorldZ;
	const float ueZ = fWorldX;

	const float fLocalX = ueX - m_fOriginX;
	const float fLocalZ = ueZ - m_fOriginZ;
	return GetNormalLocal(fLocalX, fLocalZ);
}

Vector3 HeightMapRawImage::GetTangentWorld(float fWorldX, float fWorldZ) const
{
	const float ueX = fWorldZ;
	const float ueZ = fWorldX;

	const float fLocalX = ueX - m_fOriginX;
	const float fLocalZ = ueZ - m_fOriginZ;
	return GetTangentLocal(fLocalX, fLocalZ);
}

bool HeightMapRawImage::IsInsideWorld(float fWorldX, float fWorldZ)
{
	const float ueX = fWorldZ;
	const float ueZ = fWorldX;

	const float fLocalX = ueX - m_fOriginX;
	const float fLocalZ = ueZ - m_fOriginZ;
	return IsInsideLocal(fLocalX, fLocalZ);
}

float HeightMapRawImage::SampleHeight(uint32 x, uint32 z) const
{
	const uint16 un16Raw = m_HeightMapRawData[z * m_unWidth + x];
	const int32 nSignedRaw = static_cast<int32>(un16Raw) - 32768;
	const float fHeightInCM = static_cast<float>(nSignedRaw) * (m_v3Scale.y * g_fUEHeightInverseScale);
	return fHeightInCM;
}


