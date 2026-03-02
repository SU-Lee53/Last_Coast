#include "NewCommon.hlsl"
//#include "Light.hlsl"

struct PS_GBUFFER_OUTPUT
{
	float4 RT0 : SV_Target0;	// Albedo.rgb + Metallic
	float4 RT1 : SV_Target1;	// Normal.xy + Roughness + AO
	float4 RT2 : SV_Target2;	// Emissive.rgb + Specular Power
};


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StandardShader

VS_STANDARD_OUTPUT VSStandard(VS_STANDARD_INPUT input, uint nInstanceID : SV_InstanceID)
{
	VS_STANDARD_OUTPUT output = (VS_STANDARD_OUTPUT) 0;

	matrix mtxViewProjection = mul(gCamera.mtxView, gCamera.mtxProjection);
	
	matrix mtxWorld = gWorldTransforms[nInstanceID].mtxWorld;
    
	float3 positionW = mul(float4(input.position, 1.f), mtxWorld).xyz;
	output.positionW = positionW;
	output.position = mul(float4(output.positionW, 1.f), mtxViewProjection);
	
	output.normalW = mul(float4(input.normal, 0.f), mtxWorld).xyz;
	output.tangentW = mul(float4(input.tangent, 0.f), mtxWorld).xyz;
	output.uv = input.uv;
    
	return output;
}

PS_GBUFFER_OUTPUT PSStandard(VS_STANDARD_OUTPUT input)
{
	PS_GBUFFER_OUTPUT output = (PS_GBUFFER_OUTPUT) 0;
	
	// Albedo
	float4 cAlbedo = gtxtTextures[gnTextureIndex.x].Sample(gSamplerState, input.uv);
	
	// Normal
	float3 vNormal = (gnTextureIndex.y != -1) ? ComputeNormal(input.normalW, input.tangentW, input.uv) : input.normalW;
	float2 vNormalEnc = EncodeNormalOcta(vNormal);
	
	// Materials
	MaterialData m = gMaterialData[gnMaterialIndex];
	float3 cEmissive = (gnTextureIndex.w != -1) ? gtxtTextures[gnTextureIndex.w].Sample(gSamplerState, input.uv).rgb
	                                            : m.cEmissive.rgb;
	
	float fMetallic = (gnTextureIndex.z != -1) ? gtxtTextures[gnTextureIndex.z].Sample(gSamplerState, input.uv).r
	                                           : m.fMetallic;
	
	float fRoughness = 1.0f - saturate(m.fSmoothness);
	float fAO = 1.0f;
	float fSpecularPower = m.cSpecular.a;
	
	output.RT0 = float4(cAlbedo.rgb, fMetallic);
	output.RT1 = float4(vNormalEnc, fRoughness, fAO);
	output.RT2 = float4(cEmissive, fSpecularPower);
	
	return output;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AnimatedShader

VS_SKINNED_OUTPUT VSAnimated(VS_SKINNED_INPUT input, uint nInstanceID : SV_InstanceID)
{
	VS_SKINNED_OUTPUT output = (VS_SKINNED_OUTPUT) 0;

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
		position += fWeights[i] * mul(float4(input.position, 1.f), gBoneTransforms[input.blendInices[i]]).xyz;
		normal += fWeights[i] * mul(input.normal, (float3x3) gBoneTransforms[input.blendInices[i]]);
		tangent += fWeights[i] * mul(input.tangent, (float3x3) gBoneTransforms[input.blendInices[i]]);
	}
	
	matrix mtxViewProjection = mul(gCamera.mtxView, gCamera.mtxProjection);
	matrix mtxWorld = gWorldTransforms[nInstanceID].mtxWorld;
	
	float4 positionW = mul(float4(position, 1.f), mtxWorld);
	output.positionW = positionW.xyz;
	output.position = mul(positionW, mtxViewProjection);
	
	output.normalW = mul(float4(normal, 0.f), mtxWorld).xyz;
	output.tangentW = mul(float4(tangent, 0.f), mtxWorld).xyz;
	output.uv = input.uv;
    
	return output;
}

PS_GBUFFER_OUTPUT PSAnimated(VS_SKINNED_OUTPUT input)
{
	PS_GBUFFER_OUTPUT output = (PS_GBUFFER_OUTPUT) 0;
	
	// Albedo
	float4 cAlbedo = gtxtTextures[gnTextureIndex.x].Sample(gSamplerState, input.uv);
	
	// Normal
	float3 vNormal = (gnTextureIndex.y != -1) ? ComputeNormal(input.normalW, input.tangentW, input.uv) : input.normalW;
	float2 vNormalEnc = EncodeNormalOcta(vNormal);
	
	// Materials
	MaterialData m = gMaterialData[gnMaterialIndex];
	
	float3 cEmissive = (gnTextureIndex.w != -1) ? gtxtTextures[gnTextureIndex.w].Sample(gSamplerState, input.uv).rgb
	                                            : m.cEmissive.rgb;
	
	float fMetallic = (gnTextureIndex.z != -1) ? gtxtTextures[gnTextureIndex.z].Sample(gSamplerState, input.uv).r
	                                           : m.fMetallic;
	
	float fRoughness = 1.0f - saturate(m.fSmoothness);
	float fAO = 1.0f;
	float fSpecularPower = m.cSpecular.a;
	
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
	matrix mtxWorld = gWorldTransforms[0].mtxWorld;
	
	float3 positionW = mul(float4(input.position, 1.f), mtxWorld).xyz;
	output.positionW = positionW;
	output.position = mul(float4(output.positionW, 1.f), mtxViewProjection);
	
	output.normalW = mul(float4(input.normal, 0.f), mtxWorld).xyz;
	output.tangentW = mul(float4(input.tangent, 0.f), mtxWorld).xyz;
	
	output.positionLocalXZ = input.position.zx;
	return output;
}

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
	
	return output;
}
