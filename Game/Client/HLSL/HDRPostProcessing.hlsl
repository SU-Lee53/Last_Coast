#include "NewCommon.hlsl"

float4 PSDefferedFog(VS_QUAD_OUTPUT input) : SV_Target0
{
	// Depth : gtxtGBufferDepth
	// Deffered Lighting Result : gtxtHDRResult
	
	int2 pixelPos = input.position.xy;
	float2 uv = input.position.xy / float2(gnScreenSize);
	
	float3 sceneColor = gtxtHDRResult.Load(int3(pixelPos, 0)).rgb;
	float fDepth = gtxtGBufferDepth.Load(int3(pixelPos, 0)).r;
	if (fDepth >= 0.999999f)
	{
		return float4(sceneColor, 1.0f);
	}
	
	float3 worldPos = ReconstructWorldPos(uv, fDepth);
	float3 finalColor = ApplyFog(sceneColor, worldPos);
	
	return float4(finalColor, 1.0);
}
