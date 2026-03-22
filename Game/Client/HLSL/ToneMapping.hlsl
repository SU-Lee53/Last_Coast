#include "NewCommon.hlsl"
//#include "Light.hlsl"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tone Mapping

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

VS_QUAD_OUTPUT VSToneMapping(VS_QUAD_INPUT input)
{
	VS_QUAD_OUTPUT output = (VS_QUAD_OUTPUT) 0;
	
	output.position = float4(input.position.xy, 0.f, 1.f);
	output.uv = input.uv;
	
	return output;
}

float3 ReinhardToneMap(float3 color)
{
	return color / (1.0 + color);
}

float3 GammaCorrect(float3 color)
{
	return pow(color, 1.0 / 2.2);
}

float3 ACESFilm(float3 x)
{
	float a = 2.51;
	float b = 0.03;
	float c = 2.43;
	float d = 0.59;
	float e = 0.14;
	return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

//ACES Tone mapping
float4 PSToneMapping(VS_QUAD_OUTPUT input) : SV_Target0
{
	int2 pixelPos = int2(input.uv * gnScreenSize);
	pixelPos = clamp(pixelPos, int2(0, 0), gnScreenSize - 1);
	
	float3 hdr = gtxtHDRResult.Sample(gSamplerState, input.uv).rgb;

	float3 mapped = ACESFilm(hdr);
	mapped = pow(mapped, 1.0 / 2.2);

	return float4(mapped, 1.0);
}
