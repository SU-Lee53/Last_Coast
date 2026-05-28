#include "PostProcessingCommon.hlsl"

// Kernel
static const float gWeights[9] =
{
	0.0162162162f, 0.0540540541f, 0.1216216216f,
    0.1945945946f, 0.2270270270f, 0.1945945946f,
    0.1216216216f, 0.0540540541f, 0.0162162162f
};

float3 ApplyBloomThreshold(float3 color)
{
	float brightness = max(color.r, max(color.g, color.b));

	float knee = max(gBloomThreshold * gBloomSoftKnee, 1e-5f);
	float soft = brightness - gBloomThreshold + knee;
	soft = saturate(soft / (2.0f * knee));
	soft = soft * soft * knee;

	float contribution = max(brightness - gBloomThreshold, soft);
	contribution /= max(brightness, 1e-5f);

	return color * contribution;
}

#define BLOOM_THREAD_X		16
#define BLOOM_THREAD_Y		16
#define BLOOM_RADIUS		4
#define BLOOM_KERNEL_SIZE	9

[numthreads(8,8,1)]
void CSBrightExtractDownsample(uint3 nDispatchThreadID : SV_DispatchThreadID)
{
	uint2 pixel = nDispatchThreadID.xy;
	if (pixel.x >= gBloomOutputSize.x || pixel.y >= gBloomOutputSize.y)
		return;
	
	// Half-res output pixel -> Full res 2x2 input
	int2 baseInput = int2(pixel) * 2;
	
	float3 color = 0.f;
	color += gInputTexture.Load(int3(ClampPixel(baseInput + int2(0, 0), gBloomInputSize), 0)).rgb;
	color += gInputTexture.Load(int3(ClampPixel(baseInput + int2(1, 0), gBloomInputSize), 0)).rgb;
	color += gInputTexture.Load(int3(ClampPixel(baseInput + int2(0, 1), gBloomInputSize), 0)).rgb;
	color += gInputTexture.Load(int3(ClampPixel(baseInput + int2(1, 1), gBloomInputSize), 0)).rgb;
	color *= 0.25f;
	
	color = ApplyBloomThreshold(color);
	
	gOutputTexture[pixel] = float4(color, 1.f);
}

groupshared float4 gSharedH[BLOOM_THREAD_Y][BLOOM_THREAD_X + BLOOM_RADIUS * 2];

[numthreads(BLOOM_THREAD_X, BLOOM_THREAD_Y, 1)]
void CSBloomBlurHorizontal(
	uint3 nGroupID : SV_GroupID,
	uint3 nGroupThreadID : SV_GroupThreadID,
	uint3 nDispatchThreadID : SV_DispatchThreadID)
{
	uint lx = nGroupThreadID.x;
	uint ly = nGroupThreadID.y;
	
	int2 basePixel = int2(nGroupID.xy) * int2(BLOOM_THREAD_X, BLOOM_THREAD_Y);
	
	// groupshared 16 + 8 = 24
	for (uint x = lx; x < BLOOM_THREAD_X + BLOOM_RADIUS * 2; x += BLOOM_THREAD_X)
	{
		int2 srcPixel = basePixel + int2(x - BLOOM_RADIUS, ly);
		gSharedH[ly][x] = gInputTexture.Load(int3(ClampPixel(srcPixel, gBloomInputSize), 0));
	}
	
	GroupMemoryBarrierWithGroupSync();
	
	if (nDispatchThreadID.x >= gBloomOutputSize.x || nDispatchThreadID.y >= gBloomOutputSize.y)
		return;
	
	float3 color = 0.f;
	[unroll(BLOOM_KERNEL_SIZE)]
	for (int i = -BLOOM_RADIUS; i <= BLOOM_RADIUS; ++i)
	{
		color += gSharedH[ly][lx + i + BLOOM_RADIUS].rgb * gWeights[i + BLOOM_RADIUS];
	}
	
	gOutputTexture[nDispatchThreadID.xy] = float4(color, 1.0f);
}

groupshared float4 gSharedV[BLOOM_THREAD_Y + BLOOM_RADIUS * 2][BLOOM_THREAD_X];

[numthreads(BLOOM_THREAD_X, BLOOM_THREAD_Y, 1)]
void CSBloomBlurVertical(
	uint3 nGroupID : SV_GroupID,
	uint3 nGroupThreadID : SV_GroupThreadID,
	uint3 nDispatchThreadID : SV_DispatchThreadID)
{
	uint lx = nGroupThreadID.x;
	uint ly = nGroupThreadID.y;
	
	int2 basePixel = int2(nGroupID.xy) * int2(BLOOM_THREAD_X, BLOOM_THREAD_Y);
	
	// groupshared 16 + 8 = 24
	for (uint y = ly; y < BLOOM_THREAD_Y + BLOOM_RADIUS * 2; y += BLOOM_THREAD_Y)
	{
		int2 srcPixel = basePixel + int2(lx, y - BLOOM_RADIUS);
		gSharedV[y][lx] = gInputTexture.Load(int3(ClampPixel(srcPixel, gBloomInputSize), 0));
	}
	
	GroupMemoryBarrierWithGroupSync();
	
	if (nDispatchThreadID.x >= gBloomOutputSize.x || nDispatchThreadID.y >= gBloomOutputSize.y)
		return;
	
	float3 color = 0.f;
	[unroll(BLOOM_KERNEL_SIZE)]
	for (int i = -BLOOM_RADIUS; i <= BLOOM_RADIUS; ++i)
	{
		color += gSharedV[ly + i + BLOOM_RADIUS][lx].rgb * gWeights[i + BLOOM_RADIUS];
	}
	
	gOutputTexture[nDispatchThreadID.xy] = float4(color * gBloomIntensity, 1.0f);
}
