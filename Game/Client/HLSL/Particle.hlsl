#include "NewCommon.hlsl"


struct VS_PARTICLE_INPUT
{
	float3 position : POSITION;
	float2 uv : TEXCOORD0;
};

struct VS_PARTICLE_OUTPUT
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
	float4 color : COLOR0;
	nointerpolation int textureIndex : TEXINDEX0;
};

VS_PARTICLE_OUTPUT VSParticle(VS_PARTICLE_INPUT input, uint nInstanceID : SV_InstanceID)
{
	VS_PARTICLE_OUTPUT output = (VS_PARTICLE_OUTPUT) 0;
	
	ParticleDrawData particle = gParticleData[nInstanceID];
	
	// Quad local pos
	float2 local = input.position.xy;
	
	// Particle rotation in billboard local space;
	float s, c;
	sincos(particle.fRotation, s, c);
	
	float2 rotated;
	rotated.x = local.x * c - local.y * s;
	rotated.y = local.x * s + local.y * c;
	
	float3 cameraRight = normalize(gCamera.mtxInvView._11_12_13);
	float3 cameraUp = normalize(gCamera.mtxInvView._21_22_23);
	float3 worldPos = particle.v3Position + cameraRight * rotated.x * particle.fSize + cameraUp * rotated.y * particle.fSize;
	
	float4 viewPos = mul(float4(worldPos, 1.0f), gCamera.mtxView);
	output.position = mul(viewPos, gCamera.mtxProjection);
	
	output.uv = lerp(particle.v4UVRect.xy, particle.v4UVRect.zw, input.uv);
	
	output.color = particle.v4Color;
	output.textureIndex = particle.nTextureIndex;
	
	
	return output;
}

float4 PSParticle(VS_PARTICLE_OUTPUT input) : SV_Target
{
	float4 texColor = gtxtTextures[input.textureIndex].Sample(gSamplerState, input.uv);
	float4 outColor;
	outColor.rgb = texColor.rgb * input.color.rgb;
	outColor.a = texColor.a * input.color.a;
	
	return outColor;
}
