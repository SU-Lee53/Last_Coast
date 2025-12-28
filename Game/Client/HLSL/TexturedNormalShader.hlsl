#include "Common.hlsl"
#include "Light.hlsl"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VS

VS_TEXTURED_NORMAL_TANGENT_OUTPUT VSTexturedNormal(VS_TEXTURED_NORMAL_TANGENT_INPUT input, uint nInstanceID : SV_InstanceID)
{
    VS_TEXTURED_NORMAL_TANGENT_OUTPUT output;
    
    matrix VP = mul(gCameraData.gmtxView, gCameraData.gmtxProjection);
    output.posW = mul(float4(input.pos, 1), gsbInstanceDatas[nInstanceID + gnInstanceBase]).xyz;
    output.pos = mul(float4(output.posW, 1), VP);
    output.normalW = mul(input.normal, (float3x3)gsbInstanceDatas[nInstanceID + gnInstanceBase]);
    output.tangentW = mul(input.tangent, (float3x3) gsbInstanceDatas[nInstanceID + gnInstanceBase]);
    output.uv = input.uv;
    
    return output;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PS

float3 ComputeNormal(float3 normalW, float3 tangentW, float2 uv)
{
    float3 N = normalize(normalW);
    float3 T = normalize(tangentW - dot(tangentW, N) * N);
    float3 B = cross(N, T);
    float3x3 TBN = float3x3(T, B, N);
    
    float3 normalFromMap = gtxtNormalTexture.Sample(gSamplerState, uv).rgb;
    float3 normal = (normalFromMap * 2.0f) - 1.0f; // [0, 1] ---> [-1, 1]
    
    return mul(normal, TBN);
}

float4 PSTexturedNormal(VS_TEXTURED_NORMAL_TANGENT_OUTPUT input) : SV_TARGET
{
    //return gtxtDiffuseTexture.Sample(gSamplerState, input.uv);
    float4 texColor = gtxtDiffuseTexture.Sample(gSamplerState, input.uv);
    
    float3 normal = ComputeNormal(input.normalW, input.tangentW, input.uv);
    float4 lightColor = Lighting(input.posW, normal);
    return texColor * lightColor;
}
