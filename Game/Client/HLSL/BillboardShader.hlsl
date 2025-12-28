#include "Common.hlsl"

struct VS_BILLBOARD_OUTPUT
{
    float3 centerW : POSITION;
    float2 sizeW : SIZE;
};

struct GS_BILLBOARD_OUTPUT
{
    float4 posH : SV_Position;
    float2 uv : TEXCOORD;
};

VS_BILLBOARD_OUTPUT VSBillboard(uint nVertexID : SV_VertexID)
{
    VS_BILLBOARD_OUTPUT output;
    
    output.centerW = gvPositionW;
    output.sizeW = gvBillboardSizeW;
    
    return output;
}

[maxvertexcount(4)]
void GSBillboard(point VS_BILLBOARD_OUTPUT input[1], inout TriangleStream<GS_BILLBOARD_OUTPUT> outStream)
{
    float3 vUp = float3(0, 1, 0);
    float3 vLook = gCameraData.gvCameraPosition.xyz - input[0].centerW;
    vLook = normalize(vLook);
    
    float3 vRight = cross(vUp, vLook);
    float fHalfW = input[0].sizeW.x * 0.5f;
    float fHalfH = input[0].sizeW.y * 0.5f;
    
    float4 pVertices[4];
    pVertices[0] = float4(input[0].centerW + fHalfW * vRight - fHalfH * vUp, 1.0f);
    pVertices[1] = float4(input[0].centerW + fHalfW * vRight + fHalfH * vUp, 1.0f);
    pVertices[2] = float4(input[0].centerW - fHalfW * vRight - fHalfH * vUp, 1.0f);
    pVertices[3] = float4(input[0].centerW - fHalfW * vRight + fHalfH * vUp, 1.0f);
    
    float2 pUVs[4] = { float2(0.f, 1.f), float2(0.f, 0.f), float2(1.f, 1.f), float2(1.f, 0.f) };
    
    GS_BILLBOARD_OUTPUT output;
    [unroll(4)]
    for (int i = 0; i < 4; ++i)
    {
        output.posH = mul(mul(pVertices[i], gCameraData.gmtxView), gCameraData.gmtxProjection);
        output.uv = pUVs[i];
        outStream.Append(output);
    }
}

float4 PSBillboard(GS_BILLBOARD_OUTPUT input) : SV_Target
{
    return gtxtBillboardTexture.Sample(gSamplerState, input.uv);
}
