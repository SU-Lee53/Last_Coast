#include "NewCommon.hlsl"
#include "ToneMappingCommon.hlsl"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tone Mapping

struct VS_QUAD_INPUT
{
	float3 position : POSITION;
	float2 uv : TEXCOORD0;
};

struct VS_QUAD_OUTPUT
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
};

const static uint gnLUTSize = 32;
const static float gfLUTMinEV = -10.0f;
const static float gfLUTMaxEV = 6.0f;

VS_QUAD_OUTPUT VSToneMapping(VS_QUAD_INPUT input)
{
	VS_QUAD_OUTPUT output = (VS_QUAD_OUTPUT) 0;
	
	output.position = float4(input.position.xy, 0.f, 1.f);
	output.uv = input.uv;
	
	return output;
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


float4 PSToneMapping(VS_QUAD_OUTPUT input) : SV_Target0
{
	float3 hdrColor = gtxtHDRResult.Sample(gSamplerState, input.uv).rgb;
	hdrColor *= gfExposure;
	hdrColor *= gfInputScale;
	
	if (gnDebugView == 1)
	{
		float3 finalColor = saturate(hdrColor);
		finalColor = GammaCorrect(finalColor, gfGamma);
		return float4(finalColor, 1.0f);
	}
	
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
