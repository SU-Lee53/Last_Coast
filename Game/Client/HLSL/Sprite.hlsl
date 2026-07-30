#include "NewCommon.hlsl"


struct VS_UI_QUAD_OUTPUT
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
	uint nInstance : TEXINDEX;
};

VS_UI_QUAD_OUTPUT VSUIRect(VS_QUAD_INPUT input, uint nInstanceID : SV_InstanceID)
{
	VS_UI_QUAD_OUTPUT output = (VS_UI_QUAD_OUTPUT) 0;
	
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

float4 PSUISprite(VS_UI_QUAD_OUTPUT input) : SV_Target
{
	// 라디얼 게이지: fRadialProgress >= 0 이면 12시 기준 시계방향으로 해당 비율만 그린다 (쿨다운 스타일)
	float fRadial = gUIData[input.nInstance].fRadialProgress;
	if (fRadial >= 0.f)
	{
		static const float TWO_PI = 6.28318530718f;
		float2 v2FromCenter = input.uv - float2(0.5f, 0.5f);
		float fAngle = atan2(v2FromCenter.x, -v2FromCenter.y);	// 12시 = 0, 시계방향 증가
		if (fAngle < 0.f)
			fAngle += TWO_PI;
		if (fAngle > fRadial * TWO_PI)
			discard;
	}

	// 인스턴스마다 인덱스가 달라(텍스트=0/스프라이트=1+) 웨이브 내 divergent — NonUniformResourceIndex 필수
	float4 sampledColor = gtxtTextures[NonUniformResourceIndex(gUIData[input.nInstance].nTexIndex)].Sample(gSamplerState, input.uv);
	return float4(sampledColor * gUIData[input.nInstance].v4TextColorOrTexIndex);
}
