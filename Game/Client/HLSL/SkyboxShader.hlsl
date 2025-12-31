#include "Common.hlsl"

struct VS_SKYBOX_OUTPUT
{
    float3 position : POSITION;
};

struct GS_SKYBOX_OUTPUT
{
    float4 position : SV_POSITION;
    float3 uvw : TEXCOORD0;
};

VS_SKYBOX_OUTPUT VSSkybox(uint nVertexID : SV_VertexID)
{
    VS_SKYBOX_OUTPUT output;
    
    output.position = gvCameraPosition;
    
    return output;
}

[maxvertexcount(24)]
void GSSkybox(point VS_SKYBOX_OUTPUT input[1], inout TriangleStream<GS_SKYBOX_OUTPUT> outStream)
{
    // nVertexID 에 따라서...
    // nVertexID == 0 : 앞 (+z)
    // nVertexID == 1 : 뒤 (-z)
    // nVertexID == 2 : 왼쪽 (-x)
    // nVertexID == 3 : 오른쪽 (+x)
    // nVertexID == 4 : 위 (+y)
    // nVertexID == 5 : 아래 (-y)
    
    // 기본적으로 winding order 은 동일하게 가고, Rasterizer State 에서 뒷면이 보이도록 뒤집음
    // 다만 uv는 안에서 바라봤을때 기준으로 가야 맞을듯?
    
	float fSkyboxExtent = 1.f;
    
	float3 vAxisX = float3(1.f, 0.f, 0.f);
	float3 vAxisY = float3(0.f, 1.f, 0.f);
	float3 vAxisZ = float3(0.f, 0.f, 1.f);
    
	float3 ex = vAxisX * fSkyboxExtent;
	float3 ey = vAxisY * fSkyboxExtent;
	float3 ez = vAxisZ * fSkyboxExtent;
    
	matrix mtxVP = mul(gmtxView, gmtxProjection);
    
	float4 T00 = mul(float4(input[0].position - ex + ey + ez, 1.f), mtxVP).xyww;
	float4 T01 = mul(float4(input[0].position + ex + ey + ez, 1.f), mtxVP).xyww;
	float4 T10 = mul(float4(input[0].position - ex + ey - ez, 1.f), mtxVP).xyww;
	float4 T11 = mul(float4(input[0].position + ex + ey - ez, 1.f), mtxVP).xyww;
    
	float4 B00 = mul(float4(input[0].position - ex - ey + ez, 1.f), mtxVP).xyww;
	float4 B01 = mul(float4(input[0].position + ex - ey + ez, 1.f), mtxVP).xyww;
	float4 B10 = mul(float4(input[0].position - ex - ey - ez, 1.f), mtxVP).xyww;
	float4 B11 = mul(float4(input[0].position + ex - ey - ez, 1.f), mtxVP).xyww;
    
    float4x4 VP = mul(gmtxView, gmtxProjection);
    GS_SKYBOX_OUTPUT output;
    
    // 앞 (+z)                                   
    output.position = T01; output.uvw = float3(1.f, 0.f, 0.f); outStream.Append(output);
    output.position = T00; output.uvw = float3(0.f, 0.f, 0.f); outStream.Append(output);
    output.position = B01; output.uvw = float3(1.f, 1.f, 0.f); outStream.Append(output);
    output.position = B00; output.uvw = float3(0.f, 1.f, 0.f); outStream.Append(output);
    outStream.RestartStrip();
    
    // 뒤 (-z)
    output.position = T10; output.uvw = float3(1.f, 0.f, 1.f); outStream.Append(output);
    output.position = T11; output.uvw = float3(0.f, 0.f, 1.f); outStream.Append(output);
    output.position = B10; output.uvw = float3(1.f, 1.f, 1.f); outStream.Append(output);
    output.position = B11; output.uvw = float3(0.f, 1.f, 1.f); outStream.Append(output);
    outStream.RestartStrip();
    
    // 왼쪽 (-x)
    output.position = T00; output.uvw = float3(1.f, 0.f, 2.f); outStream.Append(output);
    output.position = T10; output.uvw = float3(0.f, 0.f, 2.f); outStream.Append(output);
    output.position = B00; output.uvw = float3(1.f, 1.f, 2.f); outStream.Append(output);
    output.position = B10; output.uvw = float3(0.f, 1.f, 2.f); outStream.Append(output);
    outStream.RestartStrip();
    
    // 오른쪽 (+x)
    output.position = T11; output.uvw = float3(1.f, 0.f, 3.f); outStream.Append(output);
    output.position = T01; output.uvw = float3(0.f, 0.f, 3.f); outStream.Append(output);
    output.position = B11; output.uvw = float3(1.f, 1.f, 3.f); outStream.Append(output);
    output.position = B01; output.uvw = float3(0.f, 1.f, 3.f); outStream.Append(output);
    outStream.RestartStrip();
    
    // 위 (+y)
    output.position = T00; output.uvw = float3(0.f, 1.f, 4.f); outStream.Append(output);
    output.position = T01; output.uvw = float3(1.f, 1.f, 4.f); outStream.Append(output);
    output.position = T10; output.uvw = float3(0.f, 0.f, 4.f); outStream.Append(output);
    output.position = T11; output.uvw = float3(1.f, 1.f, 4.f); outStream.Append(output);
    outStream.RestartStrip();
    
    // 아래 (-y)
    output.position = B01; output.uvw = float3(1.f, 0.f, 5.f); outStream.Append(output);
    output.position = B00; output.uvw = float3(0.f, 0.f, 5.f); outStream.Append(output);
    output.position = B11; output.uvw = float3(1.f, 1.f, 5.f); outStream.Append(output);
    output.position = B10; output.uvw = float3(0.f, 1.f, 5.f); outStream.Append(output);
    outStream.RestartStrip();
                  
}

float4 PSSkybox(GS_SKYBOX_OUTPUT input) : SV_Target
{
    return gtxtSkyboxTextures.Sample(gSkyboxSamplerState, input.uvw);
}
