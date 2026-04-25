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
	uint nInstance : TEXINDEX;
};

VS_QUAD_OUTPUT VSUIRect(VS_QUAD_INPUT input, uint nInstanceID : SV_InstanceID)
{
	VS_QUAD_OUTPUT output = (VS_QUAD_OUTPUT) 0;
	
	// local [-1, 1] -> [0,1]
	float2 localUV;
	localUV.x = input.position.x * 0.5f + 0.5f;
	localUV.y = -input.position.y * 0.5f + 0.5f;
	
	float left = gUIData[nInstanceID].v4ScreenRect.x;
	float top = gUIData[nInstanceID].v4ScreenRect.y;
	float right = gUIData[nInstanceID].v4ScreenRect.z;
	float bottom = gUIData[nInstanceID].v4ScreenRect.w;

	// screen pixel pos
	float2 screenPos;
	screenPos.x = lerp(left, right, localUV.x);
	screenPos.y = lerp(top, bottom, localUV.y);
	
	// pixel -> NDC
	float2 ndc;
	ndc.x = (screenPos.x / gnScreenSize.x) * 2.0f - 1.0f;
	ndc.y = 1.0f - (screenPos.y / gnScreenSize.y) * 2.0f;
	
	output.position = float4(ndc, 0.f, 1.f);
	
	// localUV [0, 1] -> atlas/sprite UV rect
	output.uv.x = lerp(gUIData[nInstanceID].v4UVRect.x, gUIData[nInstanceID].v4UVRect.z, input.uv.x);
	output.uv.y = lerp(gUIData[nInstanceID].v4UVRect.y, gUIData[nInstanceID].v4UVRect.w, input.uv.y);
	
	output.nInstance = nInstanceID;
	
	return output;
}

float4 PSUISprite(VS_QUAD_OUTPUT input) : SV_Target
{
	float4 sampledColor = gtxtTextures[gUIData[input.nInstance].nTexIndex].Sample(gSamplerState, input.uv);
	return float4(sampledColor * gUIData[input.nInstance].v4TextColorOrTexIndex);
}
