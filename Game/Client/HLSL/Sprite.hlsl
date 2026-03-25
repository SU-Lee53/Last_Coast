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
	uint texIndex : TEXINDEX;
};

VS_QUAD_OUTPUT VSSprite(VS_QUAD_INPUT input, uint nInstanceID : SV_InstanceID)
{
	VS_QUAD_OUTPUT output = (VS_QUAD_OUTPUT) 0;
	
	// input : [0 ~ 1]
	float2 screenUV = 0;
	screenUV.x = lerp(gSpriteData[nInstanceID].fLeft, gSpriteData[nInstanceID].fRight, input.position.x);
	screenUV.y = lerp(gSpriteData[nInstanceID].fTop, gSpriteData[nInstanceID].fBottom, input.position.y);
	
	float2 ndcUV = 0;
	ndcUV.x = screenUV.x * 2.0f - 1.0f;
	ndcUV.y = 1.0f - screenUV.y * 2.0f;
	
	output.position = float4(ndcUV, 0.f, 1.f);
	output.uv = float2(input.uv.x, 1.0f - input.uv.y);
	output.texIndex = nInstanceID;
	
	return output;
}

float4 PSSprite(VS_QUAD_OUTPUT input) : SV_Target
{
	return gtxtTextures[input.texIndex].SampleLevel(gSamplerState, input.uv, 0);
}
