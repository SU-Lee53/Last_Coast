#include "Common.hlsl"
#include "Light.hlsl"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VS

VS_TEXTURED_NORMAL_OUTPUT VSTexturedIlluminated(VS_TEXTURED_NORMAL_INPUT input, uint nInstanceID : SV_InstanceID)
{
    VS_TEXTURED_NORMAL_OUTPUT output;
    
    matrix VP = mul(gCameraData.gmtxView, gCameraData.gmtxProjection);
    output.posW = mul(float4(input.pos, 1), gsbInstanceDatas[nInstanceID + gnInstanceBase]).xyz;
    output.pos = mul(float4(output.posW, 1), VP);
    output.normalW = mul(input.normal, (float3x3) gsbInstanceDatas[nInstanceID + gnInstanceBase]);
    output.uv = input.uv;
    
    return output;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PS

float4 PSTexturedIlluminated(VS_TEXTURED_NORMAL_OUTPUT input) : SV_TARGET
{
    float4 texColor = gtxtDiffuseTexture.Sample(gSamplerState, input.uv);
    float4 lightColor = Lighting(input.posW, input.normalW);
    return texColor + lightColor;
}
