#include "NewCommon.hlsl"

float HashAtmosphere(float2 position)
{
	position = frac(position * float2(123.34f, 456.21f));
	position += dot(position, position + 45.32f);
	return frac(position.x * position.y);
}

float ValueNoiseAtmosphere(float2 position)
{
	float2 cell = floor(position);
	float2 weight = frac(position);
	weight = weight * weight * (3.0f - 2.0f * weight);

	float bottom = lerp(HashAtmosphere(cell), HashAtmosphere(cell + float2(1.0f, 0.0f)), weight.x);
	float top = lerp(HashAtmosphere(cell + float2(0.0f, 1.0f)), HashAtmosphere(cell + 1.0f), weight.x);
	return lerp(bottom, top, weight.y);
}

float4 PSAtmosphericFogDetail(VS_QUAD_OUTPUT input) : SV_Target0
{
	int2 pixel = int2(input.position.xy);
	float4 sceneColor = gtxtHDRResult.Load(int3(pixel, 0));

	if (gfFogDetailStrength <= 0.0f)
	{
		return sceneColor;
	}

	float depth = gtxtGBufferDepth.Load(int3(pixel, 0)).r;
	float reconstructionDepth = min(depth, 0.999f);
	float3 worldPosition = ReconstructWorldPos(input.uv, reconstructionDepth);
	float3 cameraRay = worldPosition - gCamera.v3CameraPosition;
	float distanceToPixel = length(cameraRay);
	float3 viewDirection = cameraRay / max(distanceToPixel, 1e-4f);
	float distanceMask = 1.0f;
	float heightMask = 1.0f;

	if (depth < 0.9999f)
	{
		float detailFadeRange = max(500.0f, (gfFogCutOffDistance - gfFogDetailDistanceStart) * 0.45f);
		distanceMask = smoothstep(gfFogDetailDistanceStart, gfFogDetailDistanceStart + detailFadeRange, distanceToPixel);
		heightMask = 1.0f - smoothstep(gfFogBaseHeight, gfFogBaseHeight + max(gfFogDetailHeightRange, 1.0f), worldPosition.y);
	}
	else
	{
		float skyDistance = max(gfFogCutOffDistance, gfFogDetailDistanceStart + 1000.0f);
		worldPosition = gCamera.v3CameraPosition + viewDirection * skyDistance;
		heightMask = 1.0f - smoothstep(0.05f, 0.55f, abs(viewDirection.y));
	}

	float2 wind = float2(0.82f, 0.57f) * gSceneGlobal.fTotalTime * gfFogDetailNoiseSpeed;
	float2 noisePosition = worldPosition.xz * gfFogDetailNoiseScale + wind;
	float broadNoise = ValueNoiseAtmosphere(noisePosition);
	float fineNoise = ValueNoiseAtmosphere(noisePosition * 2.07f - wind * 0.65f);
	float fogNoise = smoothstep(0.26f, 0.76f, broadNoise * 0.68f + fineNoise * 0.32f);

	float dayBlend = saturate(gSkybox.fDayNightBlend);
	float3 visibleLightDirection = normalize(lerp(-gSkybox.v3SunDirection, gSkybox.v3SunDirection, step(0.5f, dayBlend)));
	float3 visibleLightColor = lerp(gSkybox.v3MoonColor, gSkybox.v3SunColor, dayBlend);
	float forwardScattering = pow(saturate(dot(viewDirection, visibleLightDirection)), 5.0f) * gfFogDetailDirectionalScattering;
	float densityLuminance = lerp(0.72f, 1.30f, fogNoise);
	float3 scatteringColor = gfogColor.rgb * densityLuminance + visibleLightColor * forwardScattering;
	float fogAmount = saturate(gfFogDetailStrength * lerp(0.35f, 1.0f, fogNoise) * distanceMask * heightMask);

	return float4(lerp(sceneColor.rgb, scatteringColor, fogAmount), sceneColor.a);
}
