#ifndef _LIGHT_
#define _LIGHT_

#include "NewCommon.hlsl"

float ComputeAttenuation(float3 attenuation, float range, float dist)
{
	float denominator = dot(attenuation, float3(1.0f, dist, dist * dist));
	float att = 1.0f / max(denominator, 1e-6f);

    // Range cutoff
	float rangeFade = saturate(1.0f - dist / range);

	return att * rangeFade;
}

float ComputeSpotFactor(float3 lightDir, float theta, float phi, float falloff, float3 Ldir)
{
    // Ldir = surface → light 방향 (정규화)
	float3 lightToSurface = -Ldir; // light → surface

	float cosAngle = dot(lightToSurface, normalize(lightDir));

    // Inner~Outer 사이 보간
	float spot =
        saturate((cosAngle - phi) / (theta - phi));

    // Falloff exponent 적용
	return pow(spot, falloff);
}

float3 DiffuseLambert(float3 normal, float3 lightDir, float3 albedo, float3 radiance)
{
	float NdotL = saturate(dot(normal, lightDir));
	return albedo * radiance * NdotL;
}

float3 SpecularBlinn(float3 normal, float3 lightDir, float3 viewDir, float specularPower, float specularIntensity, float3 radiance)
{
	float3 H = normalize(lightDir + viewDir);
	float NDotH = saturate(dot(normal, H));
	float specular = pow(NDotH, specularPower) * specularIntensity;
	return radiance * specular;
}

float3 DirectionalLight(float3 worldPos, float3 normal, float3 viewDir, float3 albedo, float specularPower, float specularIntensity, int nLightIndex)
{
	LightData lightData = gLightData[nLightIndex];
	
	float3 lightDir = -normalize(lightData.vDirection);
	//float3 lightDir = -normalize(worldPos - lightData.vPosition);
	float3 radiance = lightData.vColor * lightData.fIntensity;

	float3 diffuse = DiffuseLambert(normal, lightDir, albedo, radiance);
	float3 specular = SpecularBlinn(normal, lightDir, viewDir, specularPower, specularIntensity, radiance);
	
	return diffuse + specular;
}

float3 PointLight(float3 worldPos, float3 normal, float3 viewDir, float3 albedo, float specularPower, float specularIntensity, int nLightIndex)
{
	LightData lightData = gLightData[nLightIndex];
	
	float3 toLight = lightData.vPosition - worldPos;
	float distance = length(toLight);
	if (distance > lightData.fRange)
	{
		return float3(0.f, 0.f, 0.f);
	}
	
	float3 lightDir = toLight / max(distance, FLT_EPSILON);
	float attenuation = ComputeAttenuation(lightData.vAttenuation, lightData.fRange, distance);
	
	float3 radiance = lightData.vColor * lightData.fIntensity * attenuation;
	
	float3 diffuse = DiffuseLambert(normal, lightDir, albedo, radiance);
	float3 specular = SpecularBlinn(normal, lightDir, viewDir, specularPower, specularIntensity, radiance);
	
	return diffuse + specular;
}

float3 SpotLight(float3 worldPos, float3 normal, float3 viewDir, float3 albedo, float specularPower, float specularIntensity, int nLightIndex)
{
	LightData lightData = gLightData[nLightIndex];
	
	float3 toLight = lightData.vPosition - worldPos;
	float distance = length(toLight);
	if (distance > lightData.fRange)
	{
		return float3(0.f, 0.f, 0.f);
	}
	
	float3 lightDir = toLight / max(distance, FLT_EPSILON);
	
	float attenuation = ComputeAttenuation(lightData.vAttenuation, lightData.fRange, distance);
	float spot = ComputeSpotFactor(lightData.vDirection, lightData.fTheta, lightData.fPhi, lightData.fFalloff, lightDir);
	
	float3 radiance = lightData.vColor * lightData.fIntensity * attenuation * spot;
	
	float3 diffuse = DiffuseLambert(normal, lightDir, albedo, radiance);
	float3 specular = SpecularBlinn(normal, lightDir, viewDir, specularPower, specularIntensity, radiance);
	
	return diffuse + specular;
}

/////////////////////////////////////////////////////////////////////////////
// Shadow

int GetCascadeIndex(float fViewDepth)
{
	int idx = 0;
	
	[unroll(NUM_CASCADES)]
	for (int i = 0; i < NUM_CASCADES - 1; ++i)
	{
		idx += (fViewDepth > gCamera.gvCascadeSplits[i]) ? 1 : 0;
	}
	
	return idx;
}

float Compute3x3PCF(float3 shadowPos, int nCascadeIndex)
{
	const float fCascadeSize[4] = { 2048, 2048, 1024, 512 };
	float fSum = 0.f;
	
	const float2 texelSize = (1.0f / fCascadeSize[nCascadeIndex]).xx;
	
	[unroll]
	for (int y = -1; y <= 1; ++y)
	{
		[unroll]
		for (int x = -1; x <= 1; ++x)
		{
			float2 offset = float2(x, y) * texelSize;
			fSum += gtxtCascadeShadowMaps[nCascadeIndex].SampleCmpLevelZero(gShadowMapSamplerState, shadowPos.xy + offset, shadowPos.z);
		}
	}
	
	return fSum / 9.f;
}

float SampleCascadeShadow(float3 worldPos, int nCascadeIndex)
{
	float4 shadowPos = mul(float4(worldPos, 1.f), gmtxCascadeShadows[nCascadeIndex]);
	shadowPos.xyz /= shadowPos.w;
	return Compute3x3PCF(shadowPos.xyz, nCascadeIndex);
	//return gtxtCascadeShadowMaps[nCascadeIndex].SampleCmpLevelZero(gShadowMapSamplerState, shadowPos.xy, shadowPos.z);
}


float ComputeCascadeShadow(float3 worldPos)
{
	float3 viewPos = mul(float4(worldPos, 1.f), gCamera.mtxView).xyz;
	float fViewDepth = viewPos.z;
	
	int nCascadeIndex = GetCascadeIndex(fViewDepth);
	float fShadowA = SampleCascadeShadow(worldPos, nCascadeIndex);
	
	if (nCascadeIndex >= NUM_CASCADES - 1)
	{
		return fShadowA;
	}
	
	float fSplitStart = (nCascadeIndex == 0) ? 0.0f : gCamera.gvCascadeSplits[nCascadeIndex - 1];
	float fSplitEnd = gCamera.gvCascadeSplits[nCascadeIndex];
	float fRange = fSplitEnd - fSplitStart;
	
	float fBlendWidth = fRange * 0.10f;	// Last 10%
	float fBlendStart = fSplitEnd - fBlendWidth;
	if (fViewDepth < fBlendStart)
	{
		return fShadowA;
	}
	
	float fShadowB = SampleCascadeShadow(worldPos, nCascadeIndex + 1);
	
	float t = saturate((fViewDepth - fBlendStart) / max(fBlendWidth, FLT_EPSILON));
	return lerp(fShadowA, fShadowB, t);

}


#endif
