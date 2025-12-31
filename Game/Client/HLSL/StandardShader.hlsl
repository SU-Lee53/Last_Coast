#include "Common.hlsl"
#include "Light.hlsl"

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
