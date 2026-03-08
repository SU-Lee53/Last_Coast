#include "NewCommon.hlsl"

struct VS_SKYBOX_INPUT
{
	float3 position : POSITION;
	float2 uv : TEXCOORD0;
};

struct VS_SKYBOX_OUTPUT
{
	float4 position : SV_POSITION;
	float3 dir : TEXCOORD0;
};

VS_SKYBOX_OUTPUT VSSkybox(VS_SKYBOX_INPUT input)
{
	VS_SKYBOX_OUTPUT output = (VS_SKYBOX_OUTPUT) 0;
	
	float4x4 mtxView = gCamera.mtxView;
	mtxView._41_42_43 = float3(0, 0, 0);
	
	float4x4 VP = mul(mtxView, gCamera.mtxProjection);
	float4 posW = mul(float4(input.position, 1.0f), VP);
	
	output.position = posW.xyww;
	output.dir = input.position;
	
	return output;
}

float4 PSSkybox(VS_SKYBOX_OUTPUT input) : SV_Target
{
	float3 dir = normalize(input.dir);
	return gtxtSkyboxDay.Sample(gSkyboxSamplerState, dir);
}
