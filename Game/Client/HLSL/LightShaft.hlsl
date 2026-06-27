#include "NewCommon.hlsl"

float GetLuminanceForLightShaft(float3 color)
{
	return dot(color, float3(0.2126f, 0.7152f, 0.0722f));
}

float4 PSLightShaft(VS_QUAD_OUTPUT input) : SV_Target0
{
	if (gnEnableLightShaft == 0)
	{
		return float4(0.f, 0.f, 0.f, 1.f);
	}

	float2 uv = input.uv;
	float2 deltaUV = uv - gv2LightScreenPosition;
	float fSampleCount = max(float(gnLightShaftSampleCount), 1.0f);
	deltaUV *= gfLightShaftDensity / fSampleCount;

	float illuminationDecay = 1.0f;
	float3 shaftColor = 0.0f;

	[loop]
	for (int i = 0; i < gnLightShaftSampleCount; ++i)
	{
		uv -= deltaUV;

		if (any(uv < 0.0f) || any(uv > 1.0f))
		{
			illuminationDecay *= gfLightShaftDecay;
			continue;
		}

		float depth = gtxtGBufferDepth.SampleLevel(gSamplerState, uv, 0.0f).r;
		float skyVisibility = smoothstep(gfLightShaftDepthThreshold, 1.0f, depth);

		float3 sampleColor = gtxtHDRResult.SampleLevel(gSamplerState, uv, 0.0f).rgb;
		float luminance = GetLuminanceForLightShaft(sampleColor);
		float sourceMask = smoothstep(0.7f, 4.0f, luminance);

		shaftColor += sampleColor * sourceMask * skyVisibility * illuminationDecay * gfLightShaftWeight;
		illuminationDecay *= gfLightShaftDecay;
	}

	shaftColor *= gfLightShaftExposure * gfLightShaftIntensity;

	return float4(shaftColor, 1.f);
}
