#include "NewCommon.hlsl"
#include "Light.hlsl"

struct PS_GBUFFER_OUTPUT
{
	float4 RT0 : SV_Target0; // Albedo.rgb + Metallic
	float4 RT1 : SV_Target1; // Normal.xy + Roughness + AO
	float4 RT2 : SV_Target2; // Emissive.rgb + Specular Intensity
};

float GetSpecularIntensity(MaterialData materialData, float2 uv)
{
	float fSpecularIntensity = saturate(materialData.cSpecular.a);
	if (gnTextureIndex.w != -1)
	{
		float3 cSpecularMap = gtxtTextures[gnTextureIndex.w].Sample(gSamplerState, uv).rgb;
		fSpecularIntensity *= dot(cSpecularMap, float3(0.2126f, 0.7152f, 0.0722f));
	}
	else
	{
		fSpecularIntensity = min(fSpecularIntensity, 0.10f);
	}

	return saturate(fSpecularIntensity);
}

float GetMetallic(MaterialData materialData, float2 uv)
{
	if (gnTextureIndex.z != -1)
	{
		return saturate(gtxtTextures[gnTextureIndex.z].Sample(gSamplerState, uv).r);
	}

	return min(saturate(materialData.fMetallic), 0.05f);
}

float GetFallbackRoughness(MaterialData materialData)
{
	float fRoughness = 1.0f - saturate(materialData.fSmoothness);
	return max(fRoughness, 0.82f);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Standard VS

VS_STANDARD_OUTPUT VSStandard(VS_STANDARD_INPUT input, uint nInstanceID : SV_InstanceID)
{
	VS_STANDARD_OUTPUT output = (VS_STANDARD_OUTPUT) 0;
	int nWorldTransformBase = gnWorldTransformOffset + nInstanceID;

	matrix mtxViewProjection = mul(gCamera.mtxView, gCamera.mtxProjection);
	
	matrix mtxWorld = gWorldTransforms[nWorldTransformBase].mtxWorld;
    
	float3 positionW = mul(float4(input.position, 1.f), mtxWorld).xyz;
	output.positionW = positionW;
	output.position = mul(float4(output.positionW, 1.f), mtxViewProjection);
	
	//float3x3 mtxNormal = (float3x3) transpose(gWorldTransforms[nWorldTransformBase].mtxInvWorld);
	//output.normalW = normalize(mul(input.normal, mtxNormal));
	//output.tangentW = normalize(mul(input.tangent, mtxNormal));
	
	output.normalW = mul(float4(input.normal, 1.f), mtxWorld).xyz;
	output.tangentW = mul(float4(input.tangent, 1.f), mtxWorld).xyz;
	output.uv = input.uv;
    
	return output;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Animated VS

VS_STANDARD_OUTPUT VSAnimated(VS_SKINNED_INPUT input, uint nInstanceID : SV_InstanceID)
{
	VS_STANDARD_OUTPUT output = (VS_STANDARD_OUTPUT) 0;
	int nBoneTransformBase = gBoneTransformOffsets[nInstanceID];
	int nWorldTransformBase = gnWorldTransformOffset + nInstanceID;
	
	float fWeights[4] = { 0.f, 0.f, 0.f, 0.f };
	fWeights[0] = input.blendWeights.x;
	fWeights[1] = input.blendWeights.y;
	fWeights[2] = input.blendWeights.z;
	fWeights[3] = input.blendWeights.w;
	
	float3 position = float3(0, 0, 0);
	float3 normal = float3(0, 0, 0);
	float3 tangent = float3(0, 0, 0);
	
	[unroll(4)]
	for (int i = 0; i < 4; ++i)
	{
		position += fWeights[i] * mul(float4(input.position, 1.f), gBoneTransforms[nBoneTransformBase + input.blendInices[i]]).xyz;
		normal += fWeights[i] * mul(input.normal, (float3x3) gBoneTransforms[nBoneTransformBase + input.blendInices[i]]);
		tangent += fWeights[i] * mul(input.tangent, (float3x3) gBoneTransforms[nBoneTransformBase + input.blendInices[i]]);
	}
	
	matrix mtxViewProjection = mul(gCamera.mtxView, gCamera.mtxProjection);
	matrix mtxWorld = gWorldTransforms[nWorldTransformBase].mtxWorld;
	
	float3 positionW = mul(float4(position, 1.f), mtxWorld).xyz;
	output.positionW = positionW;
	output.position = mul(float4(positionW, 1.f), mtxViewProjection);
	
	//float3x3 mtxNormal = (float3x3) transpose(gWorldTransforms[nWorldTransformBase].mtxInvWorld);
	//output.normalW = normalize(mul(normal, mtxNormal));
	//output.tangentW = normalize(mul(tangent, mtxNormal));
	//output.uv = input.uv;
    
	//float3x3 mtxNormal = (float3x3) transpose(gWorldTransforms[nWorldTransformBase].mtxInvWorld);
	//output.normalW = normalize(mul(input.normal, mtxNormal));
	//output.tangentW = normalize(mul(input.tangent, mtxNormal));
	
	output.normalW = mul(float4(input.normal, 1.f), mtxWorld).xyz;
	output.tangentW = mul(float4(input.tangent, 1.f), mtxWorld).xyz;
	output.uv = input.uv;
	
	return output;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Common GBuffer PS

[earlydepthstencil]
PS_GBUFFER_OUTPUT PSGBufferOpaque(VS_STANDARD_OUTPUT input)
{
	PS_GBUFFER_OUTPUT output = (PS_GBUFFER_OUTPUT) 0;
	
	// Albedo
	float4 cAlbedo = gtxtTextures[gnTextureIndex.x].Sample(gSamplerState, input.uv);
	
	// Normal
	float3 vNormal = (gnTextureIndex.y != -1) ? ComputeNormal(input.normalW, input.tangentW, input.uv) : input.normalW;
	//float3 vNormal = normalize(input.normalW);
	float2 vNormalEnc = EncodeNormalOcta(vNormal);
	
	// Materials
	MaterialData m = gMaterialDatas[gnMaterialIndex];
	
	float3 cEmissive = m.cEmissive.rgb;
	
	float fMetallic = GetMetallic(m, input.uv);
	float fRoughness = GetFallbackRoughness(m);
	float fAO = 1.0f;
	float fSpecularIntensity = GetSpecularIntensity(m, input.uv);
	
	output.RT0 = float4(cAlbedo.rgb, fMetallic);
	output.RT1 = float4(vNormalEnc, fRoughness, fAO);
	output.RT2 = float4(cEmissive, fSpecularIntensity);
	
	return output;
}

PS_GBUFFER_OUTPUT PSGBufferAlphaMask(VS_STANDARD_OUTPUT input)
{
	PS_GBUFFER_OUTPUT output = (PS_GBUFFER_OUTPUT) 0;
	
	// Albedo
	float4 cAlbedo = gtxtTextures[gnTextureIndex.x].Sample(gSamplerState, input.uv);
	if (gMaterialDatas[gnMaterialIndex].eAlphaMode == ALPHA_MODE_MASKED)
	{
		clip(cAlbedo.a - gfAlphaMaskCutoff);
	}
	
	// Normal
	float3 vNormal = (gnTextureIndex.y != -1) ? ComputeNormal(input.normalW, input.tangentW, input.uv) : input.normalW;
	//float3 vNormal = normalize(input.normalW);
	float2 vNormalEnc = EncodeNormalOcta(vNormal);
	
	// Materials
	MaterialData m = gMaterialDatas[gnMaterialIndex];
	
	float3 cEmissive = m.cEmissive.rgb;
	
	float fMetallic = GetMetallic(m, input.uv);
	float fRoughness = GetFallbackRoughness(m);
	float fAO = 1.0f;
	float fSpecularIntensity = GetSpecularIntensity(m, input.uv);
	
	output.RT0 = float4(cAlbedo.rgb, fMetallic);
	output.RT1 = float4(vNormalEnc, fRoughness, fAO);
	output.RT2 = float4(cEmissive, fSpecularIntensity);
	
	return output;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// WaterShader

float SampleWaterNoise(float2 uv)
{
	float t = gSceneGlobal.fTotalTime;
	float n0 = gtxtTextures[gnTextureIndex.x].Sample(gAnisotropicSamplerState, uv * 0.12f + float2(t * 0.018f, t * 0.011f)).r;
	float n1 = gtxtTextures[gnTextureIndex.x].Sample(gAnisotropicSamplerState, uv * 0.21f + float2(-t * 0.014f, t * 0.019f)).g;
	return (n0 + n1) * 0.5f;
}

[earlydepthstencil]
PS_GBUFFER_OUTPUT PSGBufferWater(VS_STANDARD_OUTPUT input)
{
	PS_GBUFFER_OUTPUT output = (PS_GBUFFER_OUTPUT) 0;
	MaterialData m = gMaterialDatas[gnMaterialIndex];
	
	float t = gSceneGlobal.fTotalTime;
	float2 uv = input.uv;
	float noise = SampleWaterNoise(uv);
	
	float waveX = sin(uv.x * 8.0f + uv.y * 2.5f + t * 1.4f);
	float waveZ = cos(uv.y * 7.0f - uv.x * 1.8f + t * 1.1f);
	float slopeX = (waveX * 0.55f + (noise - 0.5f) * 0.9f) * 0.10f;
	float slopeZ = (waveZ * 0.55f - (noise - 0.5f) * 0.7f) * 0.10f;
	
	float3 normalW = normalize(input.normalW);
	float3 tangentW = normalize(input.tangentW - dot(input.tangentW, normalW) * normalW);
	float3 bitangentW = normalize(cross(normalW, tangentW));
	float3 waterNormalW = normalize(normalW - tangentW * slopeX - bitangentW * slopeZ);
	float2 waterNormalEnc = EncodeNormalOcta(waterNormalW);
	
	float3 baseWater = (length(m.cDiffuse.rgb) > 1e-4f) ? m.cDiffuse.rgb : float3(0.02f, 0.18f, 0.30f);
	float3 shallowTint = float3(0.05f, 0.34f, 0.42f);
	float shimmer = saturate(noise * 0.65f + (waveX + waveZ) * 0.08f + 0.25f);
	float3 albedo = lerp(baseWater, shallowTint, shimmer);
	
	float fMetallic = 0.0f;
	float fRoughness = max(1.0f - saturate(m.fSmoothness), 0.08f);
	float fAO = 1.0f;
	float fSpecularIntensity = max(saturate(m.cSpecular.a), 0.55f);
	
	output.RT0 = float4(albedo, fMetallic);
	output.RT1 = float4(waterNormalEnc, fRoughness, fAO);
	output.RT2 = float4(m.cEmissive.rgb, fSpecularIntensity);
	
	return output;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TerrainShader

VS_TERRAIN_OUTPUT VSTerrain(VS_TERRAIN_INPUT input)
{
	VS_TERRAIN_OUTPUT output = (VS_TERRAIN_OUTPUT) 0;
	
	matrix mtxViewProjection = mul(gCamera.mtxView, gCamera.mtxProjection);
	matrix mtxWorld = gmtxTerrainWorld;
	
	float3 positionW = mul(float4(input.position, 1.f), mtxWorld).xyz;
	output.positionW = positionW;
	output.position = mul(float4(output.positionW, 1.f), mtxViewProjection);
	
	output.normalW = mul(float4(input.normal, 0.f), mtxWorld).xyz;
	output.tangentW = mul(float4(input.tangent, 0.f), mtxWorld).xyz;
	
	output.positionLocalXZ = input.position.zx;
	return output;
}

[earlydepthstencil]
PS_GBUFFER_OUTPUT PSTerrain(VS_TERRAIN_OUTPUT input)
{
	PS_GBUFFER_OUTPUT output = (PS_GBUFFER_OUTPUT) 0;
	
	float flayerWeights[MAX_LAYER] = { 0.f, 0.f, 0.f, 0.f };
	
	// Albedo
	float4 cAlbedo = BlendTerrainAlbedo(input.positionLocalXZ, flayerWeights);
	
	// Normal
	//float3 vNormal = BlendTerrainNormal(
	//	input.positionLocalXZ, 
	//	flayerWeights, 
	//	normalize(input.normalW), 
	//	normalize(input.tangentW));
	float3 vNormal = input.normalW;
	float2 vNormalEnc = EncodeNormalOcta(vNormal);
	
	// Materials
	float fMetallic = 0.0f;
	float fRoughness = 0.9f;
	float fAO = 1.0f;
	float3 cEmissive = float3(0.f, 0.f, 0.f);
	float fSpecularPower = 0.04f;
	
	output.RT0 = float4(cAlbedo.rgb, fMetallic);
	output.RT1 = float4(vNormalEnc, fRoughness, fAO);
	output.RT2 = float4(cEmissive, fSpecularPower);
	//output.RT2 = float4(input.positionW, 1.0f);
	
	return output;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Deffered Lighting

float4 PSDefferedLighting(VS_QUAD_OUTPUT input) : SV_Target0
{
	int2 pixelPos = int2(input.position.xy);
	float2 uv = input.position.xy / float2(gnScreenSize);

	GBufferData g = LoadGBuffer(pixelPos);

	float3 worldPos = ReconstructWorldPos(uv, g.depth);
	float3 viewDir = normalize(gCamera.v3CameraPosition - worldPos);

	float gloss = 1.0f - saturate(g.roughness);
	float specularPower = lerp(4.f, 192.f, gloss * gloss);
	float specularIntensity = g.specular * lerp(0.05f, 1.0f, gloss * gloss);

	float fShadow = ComputeCascadeShadow(worldPos);

	float3 directLighting = 0.0f;

    [loop]
	for (int i = 0; i < gSceneGlobal.nNumLights; ++i)
	{
		LightData lightData = gLightData[i];
		if (lightData.bEnable == 0)
		{
			continue;
		}

		switch (lightData.nType)
		{
			case POINT_LIGHT:
				directLighting += PointLight(worldPos, g.normalW, viewDir, g.albedo, specularPower, specularIntensity, i);
				break;

			case SPOT_LIGHT:
				directLighting += SpotLight(worldPos, g.normalW, viewDir, g.albedo, specularPower, specularIntensity, i);
				break;

			case DIRECTIONAL_LIGHT:
				directLighting += DirectionalLight(worldPos, g.normalW, viewDir, g.albedo, specularPower, specularIntensity, i) * fShadow;
				break;
		}
	}

	float ssao = gtxtSSAOBlur.SampleLevel(gSamplerState, uv, 0.0f).r;
	ssao = saturate(ssao);

	//float ao = saturate(g.ao * ssao);

	float3 ambient = gSceneGlobal.v4GlobalAmbient.rgb * g.albedo * ssao;
	float3 finalColor = directLighting + ambient;

	return float4(finalColor, 1.0f);
}
