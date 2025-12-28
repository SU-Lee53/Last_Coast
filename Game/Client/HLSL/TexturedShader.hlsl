#include "Common.hlsl"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VS

VS_TEXTURED_OUTPUT VSTextured(VS_TEXTURED_INPUT input, uint nInstanceID : SV_InstanceID)
{
    VS_TEXTURED_OUTPUT output;
    
    matrix VP = mul(gCameraData.gmtxView, gCameraData.gmtxProjection);
    float3 posW = mul(float4(input.pos, 1), gsbInstanceDatas[nInstanceID + gnInstanceBase]).xyz;
    output.pos = mul(float4(posW, 1), VP);
    output.uv = input.uv;
    
    return output;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PS

float4 PSTextured(VS_TEXTURED_OUTPUT input) : SV_TARGET
{
    return gtxtDiffuseTexture.Sample(gSamplerState, input.uv);
}
