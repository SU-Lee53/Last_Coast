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
	float3 hdrColor = gtxtHDRResult.Sample(gSkyboxSamplerState, input.uv).rgb;
	float3 bloomColorHalf = gtxtBloomResult[0].SampleLevel(gSkyboxSamplerState, input.uv, 0.0f).rgb;
	float3 bloomColorQuater = gtxtBloomResult[1].SampleLevel(gSkyboxSamplerState, input.uv, 0.0f).rgb;
	float3 bloomColorEighth = gtxtBloomResult[2].SampleLevel(gSkyboxSamplerState, input.uv, 0.0f).rgb;
	float3 bloomColor = bloomColorHalf * 0.82f + bloomColorQuater * 0.15f + bloomColorEighth * 0.03f;
	float3 lightShaftColor = gtxtLightShaft.SampleLevel(gSkyboxSamplerState, input.uv, 0.0f).rgb;

	hdrColor += bloomColor * 0.90f;
	hdrColor += lightShaftColor;

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
	float3 mapped = gtxtToneMapLUT.SampleLevel(gSkyboxSamplerState, uvw, 0.0f).rgb;
	
	float3 lookUVW = saturate(mapped);
	lookUVW = ApplyLUTCoordScaleBias(lookUVW);
	float3 looked = gtxtGradingLUT.SampleLevel(gSkyboxSamplerState, lookUVW, 0.0f).rgb;
	
	float3 finalColor = lerp(mapped, looked, saturate(gfLookStrength));
	
	finalColor = ApplySaturation(finalColor, gfPostSaturation);
	finalColor *= gfOutputScale;
	finalColor = GammaCorrect(finalColor, gfGamma);
	
	return float4(finalColor, 1.0f);
}

float4 PSCinematicScreenFX(VS_QUAD_OUTPUT input) : SV_Target0
{
	float2 screenSize = max(float2(gnScreenSize), 1.0f.xx);
	float2 halfTexel = 0.5f / screenSize;
	float3 finalColor = gtxtHDRResult.SampleLevel(gSkyboxSamplerState, input.uv, 0.0f).rgb;
	float fLowHealthPulse = gfLowHealthFactor * lerp(0.72f, 1.0f, sin(gSceneGlobal.fTotalTime * 5.0f) * 0.5f + 0.5f);
	float fChromaticPixels = min(gfChromaticAberration + gfDamagePulse * 1.35f + fLowHealthPulse * 0.50f, 15.0f);
	if (fChromaticPixels > 0.001f)
	{
		float2 centeredUV = input.uv - 0.5f;
		float fRadial = smoothstep(0.04f, 0.85f, length(centeredUV * 2.0f));
		float2 direction = centeredUV / max(length(centeredUV), 1e-4f);
		float2 chromaticOffset = direction * fRadial * fChromaticPixels / screenSize;
		float fEdgeDistance = min(min(input.uv.x, 1.0f - input.uv.x), min(input.uv.y, 1.0f - input.uv.y));
		float fEdgeFade = smoothstep(0.0f, max(max(abs(chromaticOffset.x), abs(chromaticOffset.y)) * 2.0f, max(halfTexel.x, halfTexel.y)), fEdgeDistance);
		chromaticOffset *= fEdgeFade;
		float2 redUV = clamp(input.uv + chromaticOffset, halfTexel, 1.0f.xx - halfTexel);
		float2 blueUV = clamp(input.uv - chromaticOffset, halfTexel, 1.0f.xx - halfTexel);
		finalColor.r = gtxtHDRResult.SampleLevel(gSkyboxSamplerState, redUV, 0.0f).r;
		finalColor.b = gtxtHDRResult.SampleLevel(gSkyboxSamplerState, blueUV, 0.0f).b;
	}

	float3 bloomColorQuater = gtxtBloomResult[1].SampleLevel(gSkyboxSamplerState, input.uv, 0.0f).rgb;
	float3 bloomColorEighth = gtxtBloomResult[2].SampleLevel(gSkyboxSamplerState, input.uv, 0.0f).rgb;
	float3 wideBloom = bloomColorQuater * 0.65f + bloomColorEighth * 0.35f;
	float3 halationSource = max(wideBloom - 0.065f.xxx, 0.0f.xxx);
	float3 halationColor = 1.0f.xxx - exp(-halationSource * 2.2f);
	finalColor += halationColor * float3(1.0f, 0.22f, 0.04f) * gfHalationStrength * 0.65f;

	float vignette = ComputeVignette(input.uv);
	finalColor *= vignette;

	float grain = ComputeFilmGrain(input.uv);
	finalColor += grain * gfGrainStrength;

	float fDamageFeedback = saturate(gfDamagePulse + fLowHealthPulse);
	float2 damagePosition = input.uv * 2.0f - 1.0f;
	float fDamageEdge = smoothstep(0.32f, 1.18f, length(damagePosition));
	float fDamageMask = saturate(fDamageFeedback * fDamageEdge * gfDamageVignetteStrength);
	float fDamageLuminance = GetLuminance(finalColor);
	finalColor = lerp(finalColor, fDamageLuminance.xxx, fDamageMask * 0.18f);
	finalColor *= lerp(1.0f.xxx, float3(1.0f, 0.68f, 0.62f), fDamageMask);
	finalColor += float3(0.07f, 0.002f, 0.0f) * fDamageMask;
	
	return float4(saturate(finalColor), 1.0f);
}
