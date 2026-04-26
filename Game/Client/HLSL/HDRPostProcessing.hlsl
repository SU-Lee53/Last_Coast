#include "NewCommon.hlsl"


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

VS_QUAD_OUTPUT VSDefferedFog(VS_QUAD_INPUT input)
{
	VS_QUAD_OUTPUT output = (VS_QUAD_OUTPUT) 0;
	
	output.position = float4(input.position.xy, 0.f, 1.f);
	output.uv = input.uv;
	
	return output;
}

//cbuffer FogData : register(b4, space0)
//{
//	float4 gfogColor;
//	float gfFogDensity;
//	float gfFogHeightFalloff;
//	float gfFogBaseHeight;
//	float gfFogStartDistance;
//	float gfFogMaxOpacity;
//	float gfFogCutoffDistance;
//	float2 pad;
//};

float4 PSDefferedFog(VS_QUAD_OUTPUT input) : SV_Target0
{
	// Depth : gtxtGBufferDepth
	// Deffered Lighting Result : gtxtHDRResult
	
	int2 pixelPos = input.position.xy;
	float2 uv = input.position.xy / float2(gnScreenSize);
	
	float3 sceneColor = gtxtHDRResult.Load(int3(pixelPos, 0)).rgb;
	float fDepth = gtxtGBufferDepth.Load(int3(pixelPos, 0)).r;
	
	float3 worldPos = ReconstructWorldPos(uv, fDepth);
	float3 finalColor = ApplyFog(sceneColor, worldPos);
	
	return float4(finalColor, 1.0);
}
