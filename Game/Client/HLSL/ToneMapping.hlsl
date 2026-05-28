#include "NewCommon.hlsl"
#include "ToneMappingCommon.hlsl"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tone Mapping

const static uint gnLUTSize = 32;
const static float gfLUTMinEV = -10.0f;
const static float gfLUTMaxEV = 6.0f;

float Hash12(float2 p)
{
	float3 p3 = frac(float3(p.xyx) * 0.1031f);
	p3 += dot(p3, p3.yzx + 33.33f);
	return frac((p3.x + p3.y) * p3.z);
}

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

float ComputeVignette(float2 uv)
{
	float2 p = uv * 2.0f - 1.0f;
	float dist = length(p);

	float vig = 1.0f - smoothstep(gfVignetteRadius, gfVignetteRadius + gfVignetteSoftness, dist);

	return lerp(1.0f, vig, saturate(gfVignetteStrength));
}

float ComputeFilmGrain(float2 uv)
{
	float2 pixel = uv * gnScreenSize.xy;
	float n = Hash12(pixel * gfGrainScale + gSceneGlobal.fTotalTime * 60.0f);
	return n * 2.0f - 1.0f;
}

float4 PSToneMapping(VS_QUAD_OUTPUT input) : SV_Target0
{
	float3 hdrColor = gtxtHDRResult.Sample(gSamplerState, input.uv).rgb;
	float3 bloomColorHalf = gtxtBloomResult[0].SampleLevel(gSamplerState, input.uv, 0.0f).rgb;
	float3 bloomColorQuater = gtxtBloomResult[1].SampleLevel(gSamplerState, input.uv, 0.0f).rgb;
	float3 bloomColorEighth = gtxtBloomResult[2].SampleLevel(gSamplerState, input.uv, 0.0f).rgb;
	float3 bloomColor = bloomColorHalf * 0.75 + bloomColorQuater * 0.20 + bloomColorEighth * 0.05;
	
    hdrColor += bloomColor;
	hdrColor += bloomColor;

	float autoExposure = 1.0f;

	if (gnEnableAutoExposure != 0)
	{
		float avgLogLum = gtxtLuminance.Load(int3(0, 0, 0)).r;
		float avgLum = exp2(avgLogLum);

		autoExposure = gfTargetLuminance / max(avgLum, 1e-4f);
		autoExposure = clamp(autoExposure, gfMinExposure, gfMaxExposure);
	}

	hdrColor *= gfExposure * autoExposure;
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
	
	// Apply ScreenFX(Grain, Vignette)
	finalColor = ApplySaturation(finalColor, gfPostSaturation);
	finalColor *= gfOutputScale;

	float vignette = ComputeVignette(input.uv);
	finalColor *= vignette;

	float grain = ComputeFilmGrain(input.uv);
	finalColor += grain * gfGrainStrength;
	
	finalColor = GammaCorrect(finalColor, gfGamma);
	
	return float4(finalColor, 1.0f);
}
