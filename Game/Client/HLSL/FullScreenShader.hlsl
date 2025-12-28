#include "Common.hlsl"

struct VS_FULLSCREEN_INPUT
{
    float4 pos : POSITION;
};

struct VS_FULLSCREEN_OUTPUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

VS_FULLSCREEN_OUTPUT VSFullScreen(VS_FULLSCREEN_INPUT input)
{
    VS_FULLSCREEN_OUTPUT output;
    
    //output.pos.xy = input.pos.xy * float2(gfWidth, gfHeight);
    //output.pos.zw = input.pos.zw;
    output.pos = float4(input.pos.xyz, 1);
    output.uv = float2((input.pos.xy * float2(0.5, 0.5)) + float2(0.5, 0.5));
    output.uv.y = -output.uv.y;
    
    return output;
}

float4 PSFullScreen(VS_FULLSCREEN_OUTPUT input) : SV_Target
{
    return gtxtDiffuseTexture.Sample(gSamplerState, input.uv);
}
