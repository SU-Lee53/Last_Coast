
struct VS_SPRITE_OUTPUT
{
    float4 pos : SV_POSITION;
};

struct GS_SPRITE_OUTPUT
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD0;
};

cbuffer TEXTURE_INFO : register(b0)
{
    float gnLeft;
    float gnTop;
    float gnRight;
    float gnBottom;
};

Texture2D gtxtTexture : register(t0);
SamplerState gSamplerState : register(s0);

VS_SPRITE_OUTPUT VSSprite(uint nVertexID : SV_VertexID)
{
    VS_SPRITE_OUTPUT output;
    
    output.pos = float4(0, 0, 0, 1);
    
    return output;
}

[maxvertexcount(4)]
void GSSprite(point VS_SPRITE_OUTPUT input[1], inout TriangleStream<GS_SPRITE_OUTPUT> outStream)
{
    float fLeft = (gnLeft * 2.f) - 1.f;
    float fTop = 1.f - (gnTop * 2.f);
    float fRight = (gnRight * 2.f) - 1.f;
    float fBottom = 1.f - (gnBottom * 2.f);
    
    GS_SPRITE_OUTPUT vertex;
    
    vertex.pos = float4(fLeft, fTop, 0.f, 1.f);
    vertex.uv = float2(0.f, 0.f);
    outStream.Append(vertex);
    
    vertex.pos = float4(fRight, fTop, 0.f, 1.f);
    vertex.uv = float2(1.f, 0.f);
    outStream.Append(vertex);
    
    vertex.pos = float4(fLeft, fBottom, 0.f, 1.f);
    vertex.uv = float2(0.f, 1.f);
    outStream.Append(vertex);
    
    vertex.pos = float4(fRight, fBottom, 0.f, 1.f);
    vertex.uv = float2(1.f, 1.f);
    outStream.Append(vertex);
}

float4 PSSprite(GS_SPRITE_OUTPUT input) : SV_Target
{
    return gtxtTexture.Sample(gSamplerState, input.uv);
}
