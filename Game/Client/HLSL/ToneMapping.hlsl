#include "NewCommon.hlsl"
#include "ToneMappingCommon.hlsl"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tone Mapping

const static uint gnLUTSize = 32;
const static float gfLUTMinEV = -10.0f;
const static float gfLUTMaxEV = 6.0f;

float GetLuminance(float3 color)
{
	return dot(color, float3(0.2126f, 0.7152f, 0.0722f));
}

float3 ApplySaturation(float3 color, float fSaturation)
{
	float luma = GetLuminance(color);
	return lerp(luma.xxx, color, fSaturation);
}

float3 GammaCorrect(float3 color, float fGamma)
{
	return pow(saturate(color), 1.0 / max(fGamma, 1e-6));
}

float3 HDRToLUTUVW(float3 hdrColor)
{
	float3 c = max(hdrColor, 1e-6f.xxx);
	float3 ev = log2(c);
	return saturate((ev - gfLUTMinEV.xxx) / (gfLUTMaxEV - gfLUTMinEV));
}

float3 ApplyLUTCoordScaleBias(float3 uvw)
{
	float fScale = (float(gnLUTSize) - 1.0f) / float(gnLUTSize);
	float fBias = 0.5f / float(gnLUTSize);
	return uvw * fScale + fBias;
}

float4 PSToneMapping(VS_QUAD_OUTPUT input) : SV_Target0
{
	float3 hdrColor = gtxtHDRResult.Sample(gSamplerState, input.uv).rgb;
	float3 bloomColorHalf = gtxtBloomResult[0].SampleLevel(gSamplerState, input.uv, 0.0f).rgb;
	float3 bloomColorQuater = gtxtBloomResult[1].SampleLevel(gSamplerState, input.uv, 0.0f).rgb;
	float3 bloomColorEighth = gtxtBloomResult[2].SampleLevel(gSamplerState, input.uv, 0.0f).rgb;
	float3 bloomColor = bloomColorHalf * 0.75 + bloomColorQuater * 0.20 + bloomColorEighth * 0.05;
	
    hdrColor += bloomColor;

    hdrColor *= gfExposure;
    hdrColor *= gfInputScale;

	
	float3 uvw = HDRToLUTUVW(hdrColor);
	uvw = ApplyLUTCoordScaleBias(uvw);
	float3 mapped = gtxtToneMapLUT.SampleLevel(gSamplerState, uvw, 0.0f).rgb;
	
	float3 lookUVW = saturate(mapped);
	lookUVW = ApplyLUTCoordScaleBias(lookUVW);
	float3 looked = gtxtGradingLUT.SampleLevel(gSamplerState, lookUVW, 0.0f).rgb;
	
	float3 finalColor = lerp(mapped, looked, saturate(gfLookStrength));
	
	finalColor = ApplySaturation(finalColor, gfPostSaturation);
	finalColor *= gfOutputScale;
	finalColor = GammaCorrect(finalColor, gfGamma);
	
	return float4(finalColor, 1.0f);
}
