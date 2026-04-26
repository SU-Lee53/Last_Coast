#include "NewCommon.hlsl"
#include "Light.hlsl"

struct VS_POSITION_INPUT
{
	float3 position : POSITION;
};

struct VS_SKINNED_POSITION_INPUT
{
	float3 position : POSITION;
	uint4 blendInices : BLENDINDICES;
	float4 blendWeights : BLENDWEIGHTS;
};

struct VS_POSITION_OUTPUT
{
	float4 position : SV_POSITION;
};

VS_POSITION_OUTPUT VSShadowStandard(VS_POSITION_INPUT input, uint nInstanceID : SV_InstanceID)
{
	VS_POSITION_OUTPUT output = (VS_POSITION_OUTPUT) 0;
	int nWorldTransformBase = gnWorldTransformOffset + nInstanceID;

	matrix mtxWorld = gWorldTransforms[nWorldTransformBase].mtxWorld;
    
	float3 positionW = mul(float4(input.position, 1.f), mtxWorld).xyz;
	output.position = mul(float4(positionW, 1.f), gmtxLightViewProj);
	
	return output;
}

VS_POSITION_OUTPUT VSShadowTerrain(VS_POSITION_INPUT input, uint nInstanceID : SV_InstanceID)
{
	VS_POSITION_OUTPUT output = (VS_POSITION_OUTPUT) 0;
	matrix mtxWorld = gmtxTerrainWorld;
    
	float3 positionW = mul(float4(input.position, 1.f), mtxWorld).xyz;
	output.position = mul(float4(positionW, 1.f), gmtxLightViewProj);
	
	return output;
}

VS_POSITION_OUTPUT VSShadowAnimated(VS_SKINNED_POSITION_INPUT input, uint nInstanceID : SV_InstanceID)
{
	VS_POSITION_OUTPUT output = (VS_POSITION_OUTPUT) 0;
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
	}
	
	matrix mtxWorld = gWorldTransforms[nWorldTransformBase].mtxWorld;
	
	float3 positionW = mul(float4(position, 1.f), mtxWorld).xyz;
	output.position = mul(float4(positionW, 1.f), gmtxLightViewProj);
	
	return output;
}
