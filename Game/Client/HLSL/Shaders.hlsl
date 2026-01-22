#include "Common.hlsl"
#include "Light.hlsl"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StandardShader

VS_STANDARD_OUTPUT VSStandard(VS_STANDARD_INPUT input, uint nInstanceID : SV_InstanceID)
{
	VS_STANDARD_OUTPUT output = (VS_STANDARD_OUTPUT) 0;

	matrix mtxViewProjection = mul(gmtxView, gmtxProjection);
	
	matrix mtxWorld = gnInstanceBase == -1 ? gmtxWorld : gsbInstanceDatas[gnInstanceBase + nInstanceID];
    
	float3 positionW = mul(float4(input.position, 1.f), mtxWorld).xyz;
	output.positionW = positionW;
	output.position = mul(float4(output.positionW, 1.f), mtxViewProjection);
	
	output.normalW = mul(float4(input.normal, 0.f), mtxWorld).xyz;
	output.tangentW = mul(float4(input.tangent, 0.f), mtxWorld).xyz;
	output.uv = input.uv;
    
	return output;
}

float4 PSStandard(VS_STANDARD_OUTPUT input) : SV_Target
{
	return gtxtDiffuseTexture.Sample(gSamplerState, input.uv);
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
		position += fWeights[i] * mul(float4(input.position, 1.f), boneTransforms[input.blendInices[i]]).xyz;
		normal += fWeights[i] * mul(input.normal, (float3x3)boneTransforms[input.blendInices[i]]);
		tangent += fWeights[i] * mul(input.tangent, (float3x3)boneTransforms[input.blendInices[i]]);
	}
	
	matrix mtxViewProjection = mul(gmtxView, gmtxProjection);
	matrix mtxWorld = gmtxWorld;
	
	float4 positionW = mul(float4(position, 1.f), mtxWorld);
	output.positionW = positionW.xyz;
	output.position = mul(positionW, mtxViewProjection);
	
	output.normalW = mul(float4(normal, 0.f), mtxWorld).xyz;
	output.tangentW = mul(float4(tangent, 0.f), mtxWorld).xyz;
	output.uv = input.uv;
    
	return output;
}

float4 PSAnimated(VS_SKINNED_OUTPUT input) : SV_Target
{
	return gtxtDiffuseTexture.Sample(gSamplerState, input.uv);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TerrainShader

VS_TERRAIN_OUTPUT VSTerrain(VS_TERRAIN_INPUT input)
{
	VS_TERRAIN_OUTPUT output = (VS_TERRAIN_OUTPUT) 0;

	matrix mtxViewProjection = mul(gmtxView, gmtxProjection);
	float3 positionW = mul(float4(input.position, 1.f), gmtxWorld).xyz;
	output.positionW = positionW;
	output.position = mul(float4(output.positionW, 1.f), mtxViewProjection);
	
	output.normalW = mul(float4(input.normal, 0.f), gmtxWorld).xyz;
	output.tangentW = mul(float4(input.tangent, 0.f), gmtxWorld).xyz;
    
	return output;
}

float4 PSTerrain(VS_TERRAIN_OUTPUT input) : SV_Target
{
	//float4 cAlbedoColor[4];
	//[unroll(4)]
	//for (int i = 0; i < 4; ++i)
	//{
	//	float2 vTileUV;
	//	vTileUV.x = (input.positionW.x - gvComponentOriginXZ.x) * gvLayerTiling[gLayerIndex[i]];
	//	vTileUV.y = (input.positionW.z - gvComponentOriginXZ.y) * gvLayerTiling[gLayerIndex[i]];
	//
	//	cAlbedoColor[i] = gtxtTerrainAlbedo[gLayerIndex[i]].Sample(gSamplerState, vTileUV);
	//}
	//
	//float2 vWeightUV;
	//vWeightUV.x = input.positionW.x / gvComponentSizeXZ.x;
	//vWeightUV.y = input.positionW.z / gvComponentSizeXZ.y;
	//
	//float4 vWeight = gtxtComponentWeightMap.Sample(gSamplerState, vWeightUV);
	//float4 cFinalColor = (cAlbedoColor[0] * vWeight.r) + (cAlbedoColor[1] * vWeight.g) + (cAlbedoColor[2] * vWeight.b) + (cAlbedoColor[3] * vWeight.a);
	//
	//// Normalize
	//float fSum = vWeight.r + vWeight.g + vWeight.b + vWeight.a;
	//if (fSum < 1e-6f)
	//{
	//	cFinalColor /= fSum;
	//}
	
	return float4(input.normalW, 1.f);
}
