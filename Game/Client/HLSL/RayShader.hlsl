#include "EffectCommon.hlsl"

VS_PARTICLE_INPUT VSRay()
{
    VS_PARTICLE_OUTPUT output;
    
    output.startPos = gvRayPositionW;
    output.direction = gvRaDirectionW;
    
    return output;
}

[maxvertexcount(24)]    // 6개의 면, 면당 정점 4개(strip)
void GSRay(point VS_PARTICLE_OUTPUT input[1], inout TriangleStream<GS_PARTICLE_OUTPUT> outStream)
{
    float4x4 W = gsbInstanceDatas[gnInstanceBase];
    float4x4 VP = mul(gCameraData.gmtxView, gCameraData.gmtxProjection);
    float4x4 WVP = mul(W, VP);
    
    // 1. 축
    // Ray 가 플레이어를 따라 돌지 않도록 Up과 Right 를 월드 기준으로 맞춤
    float3 dir = normalize(input[0].direction);
    float3 worldUp = float3(0, 1, 0);
    float3 worldRight = float3(1, 0, 0);
    worldUp = normalize(cross(dir, worldRight));
    
    // Ray 시작과 끝
    float3 p0 = float3(0, 0, 0);
    float3 p1 = p0 + float3(0, 0, 1) * gfRayLength;
    
    // 위로 너무 많이 보면 Y축이랑 평행해지니까, 이때는 X축을 up으로 쓴다
    if (abs(dot(dir, worldUp)) > 0.99f)
    {
        worldUp = float3(0, 0, 1); // or (1,0,0) / (0,0,1) 아무거나
    }
    
    // 좌우 오프셋
    float3 rx = worldRight * gfRayHalfWidth;
    float3 ry = worldUp * gfRayHalfWidth;
    
    // bottom
    float3 b00 = p0 - rx - ry; // x-, y-
    float3 b10 = p0 + rx - ry; // x+, y-
    float3 b11 = p0 + rx + ry; // x+, y+
    float3 b01 = p0 - rx + ry; // x-, y+
    
    // top
    float3 t00 = p1 - rx - ry;
    float3 t10 = p1 + rx - ry;
    float3 t11 = p1 + rx + ry;
    float3 t01 = p1 - rx + ry;
    
    GS_RAY_OUTPUT output;
    
    // bottom (Ray 시작)
    output.posH = mul(float4(b01, 1), WVP); outStream.Append(output);
    output.posH = mul(float4(b00, 1), WVP); outStream.Append(output);
    output.posH = mul(float4(b11, 1), WVP); outStream.Append(output);
    output.posH = mul(float4(b10, 1), WVP); outStream.Append(output);
    outStream.RestartStrip();
    
    // Top (Ray 끝)
    output.posH = mul(float4(t00, 1), WVP); outStream.Append(output);
    output.posH = mul(float4(t01, 1), WVP); outStream.Append(output);
    output.posH = mul(float4(t10, 1), WVP); outStream.Append(output);
    output.posH = mul(float4(t11, 1), WVP); outStream.Append(output);
    outStream.RestartStrip();
    
    // 오른쪽
    output.posH = mul(float4(b11, 1), WVP); outStream.Append(output);
    output.posH = mul(float4(b10, 1), WVP); outStream.Append(output);
    output.posH = mul(float4(t11, 1), WVP); outStream.Append(output);
    output.posH = mul(float4(t10, 1), WVP); outStream.Append(output);
    outStream.RestartStrip();
    
    // 왼쪽
    output.posH = mul(float4(b00, 1), WVP); outStream.Append(output);
    output.posH = mul(float4(b01, 1), WVP); outStream.Append(output);
    output.posH = mul(float4(t00, 1), WVP); outStream.Append(output);
    output.posH = mul(float4(t01, 1), WVP); outStream.Append(output);
    outStream.RestartStrip();
    
    // 위쪽
    output.posH = mul(float4(b11, 1), WVP); outStream.Append(output);
    output.posH = mul(float4(b01, 1), WVP); outStream.Append(output);
    output.posH = mul(float4(t11, 1), WVP); outStream.Append(output);
    output.posH = mul(float4(t01, 1), WVP); outStream.Append(output);
    outStream.RestartStrip();
  
    // 아래쪽
    output.posH = mul(float4(b00, 1), WVP); outStream.Append(output);
    output.posH = mul(float4(b10, 1), WVP); outStream.Append(output);
    output.posH = mul(float4(t00, 1), WVP); outStream.Append(output);
    output.posH = mul(float4(t10, 1), WVP); outStream.Append(output);
    outStream.RestartStrip();
    
}

float4 PSRay(GS_RAY_OUTPUT input) : SV_Target
{
    return float4(1.f, 0.f, 0.f, 1.f);  // Red
}
