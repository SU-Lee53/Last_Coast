#include "Common.hlsl"

VS_DIFFUSED_OUTPUT VSDiffused(VS_DIFFUSED_INPUT input)
{
    VS_DIFFUSED_OUTPUT output;
    
    matrix VP = gCameraData.gmtxView * gCameraData.gmtxProjection;
    
    output.pos = mul(mul(float4(input.pos, 1), mtxWorld), VP);
    output.color = input.color;
    
    return output;
}

float4 PSDiffused(VS_DIFFUSED_OUTPUT input) : SV_TARGET
{
    return input.color;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

VS_DIFFUSED_OUTPUT VSDiffusedInstanced(VS_DIFFUSED_INPUT input, uint nInstanceID : SV_InstanceID)
{
    VS_DIFFUSED_OUTPUT output;
    
    //matrix VP = gCameraData.gmtxView * gCameraData.gmtxProjection;
    
    float4 posW = mul(float4(input.pos, 1.0f), gsbInstanceDatas[nInstanceID + gnInstanceBase]);
    
    output.pos = mul(mul(posW, gCameraData.gmtxView), gCameraData.gmtxProjection);
    output.color = input.color;
    
    return output;
}
