#include "NewCommon.hlsl"
#include "Light.hlsl"

struct PS_GBUFFER_OUTPUT
{
	float4 RT0 : SV_Target0;	// Albedo.rgb + Metallic
	float4 RT1 : SV_Target1;	// Normal.xy + Roughness + AO
	float4 RT2 : SV_Target2; // Specular.rgb + Specular Power
};


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
	
	float3x3 mtxNormal = (float3x3) transpose(gWorldTransforms[nWorldTransformBase].mtxInvWorld);
	output.normalW = normalize(mul(input.normal, mtxNormal));
	output.tangentW = normalize(mul(input.tangent, mtxNormal));
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
	
	float3x3 mtxNormal = (float3x3) transpose(gWorldTransforms[nWorldTransformBase].mtxInvWorld);
	output.normalW = normalize(mul(normal, mtxNormal));
	output.tangentW = normalize(mul(tangent, mtxNormal));
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
	
	float3 cEmissive = (gnTextureIndex.w != -1) ? gtxtTextures[gnTextureIndex.w].Sample(gSamplerState, input.uv).rgb
	                                            : m.cEmissive.rgb;
	
	float fMetallic = (gnTextureIndex.z != -1) ? gtxtTextures[gnTextureIndex.z].Sample(gSamplerState, input.uv).r
	                                           : m.fMetallic;
	
	//float fRoughness = 1.0f - saturate(m.fSmoothness);
	float fRoughness = saturate(m.fSmoothness);
	float fAO = 1.0f;
	float fSpecularPower = saturate(m.cSpecular.a);
	
	output.RT0 = float4(cAlbedo.rgb, fMetallic);
	output.RT1 = float4(vNormalEnc, fRoughness, fAO);
	output.RT2 = float4(cEmissive, fSpecularPower);
	
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
	
	float3 cEmissive = (gnTextureIndex.w != -1) ? gtxtTextures[gnTextureIndex.w].Sample(gSamplerState, input.uv).rgb
	                                            : m.cEmissive.rgb;
	
	float fMetallic = (gnTextureIndex.z != -1) ? gtxtTextures[gnTextureIndex.z].Sample(gSamplerState, input.uv).r
	                                           : m.fMetallic;
	
	//float fRoughness = 1.0f - saturate(m.fSmoothness);
	float fRoughness = saturate(m.fSmoothness);
	float fAO = 1.0f;
	float fSpecularPower = saturate(m.cSpecular.a);
	
	output.RT0 = float4(cAlbedo.rgb, fMetallic);
	output.RT1 = float4(vNormalEnc, fRoughness, fAO);
	output.RT2 = float4(cEmissive, fSpecularPower);
	
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
	float3 vNormal = BlendTerrainNormal(
		input.positionLocalXZ, 
		flayerWeights, 
		normalize(input.normalW), 
		normalize(input.tangentW));
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

VS_QUAD_OUTPUT VSDefferedLighting(VS_QUAD_INPUT input)
{
	VS_QUAD_OUTPUT output = (VS_QUAD_OUTPUT) 0;
	
	output.position = float4(input.position.xy, 0.f, 1.f);
	output.uv = input.uv;
	
	return output;
}

float4 PSDefferedLighting(VS_QUAD_OUTPUT input) : SV_Target0
{
	int2 pixelPos = int2(input.position.xy);
	float2 uv = (input.position.xy) / float2(gnScreenSize);
	
	GBufferData g = LoadGBuffer(pixelPos);
	
	float3 worldPos = ReconstructWorldPos(uv, g.depth);
	float3 viewDir = normalize(gCamera.v3CameraPosition - worldPos);
	
	float specularIntensity = g.specular;
	
	float gloss = 1.0f - saturate(g.roughness);
	float specularPower = lerp(8.f, 256.f, gloss * gloss);
	
	float fShadow = ComputeCascadeShadow(worldPos);
	
	float3 finalColor = 0;
	
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
				finalColor += PointLight(worldPos, g.normalW, viewDir, g.albedo, specularPower, specularIntensity, i);
				break;
		
			case SPOT_LIGHT:
				finalColor += SpotLight(worldPos, g.normalW, viewDir, g.albedo, specularPower, specularIntensity, i);
				break;
			
			case DIRECTIONAL_LIGHT:
				finalColor += DirectionalLight(worldPos, g.normalW, viewDir, g.albedo, specularPower, specularIntensity, i) * fShadow;
				break;
		}
	}
	
	finalColor *= g.ao;
	//finalColor += g.emissive;
	finalColor += gSceneGlobal.v4GlobalAmbient.rgb;
	
	//return float4(g.albedo, 1.0f);
	//return float4(g.normalW, 1.0f);
	return float4(finalColor, 1.0f);
}
