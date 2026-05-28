#include "PostProcessingCommon.hlsl"

#define THREAD_X 16
#define THREAD_Y 16
#define TILE_X   32
#define TILE_Y   32

groupshared float gSharedLuminance[TILE_Y][TILE_X];

[numthreads(THREAD_X, THREAD_Y, 1)]
void CSExtractLuminance(
    uint3 nGroupID : SV_GroupID,
    uint3 nGroupThreadID : SV_GroupThreadID,
    uint3 nDispatchID : SV_DispatchThreadID)
{
	uint lx = nGroupThreadID.x;
	uint ly = nGroupThreadID.y;

	uint2 outPixel = nDispatchID.xy;

	int2 baseInput = int2(nGroupID.xy) * int2(TILE_X, TILE_Y);
	int2 localBase = int2(lx, ly) * 2;

    // 16x16 threads fills 32x32 input tile
    [unroll]
	for (uint oy = 0; oy < 2; ++oy)
	{
        [unroll]
		for (uint ox = 0; ox < 2; ++ox)
		{
			int2 local = localBase + int2(ox, oy);
			int2 src = baseInput + local;

			float3 hdr = gInputTexture.Load(int3(ClampPixel(src, gAutoExposureInputSize), 0)).rgb;
			float lum = ComputeLuminance(hdr);
			
			gSharedLuminance[local.y][local.x] = log2(max(lum, 1e-4f));
		}
	}

	GroupMemoryBarrierWithGroupSync();

	if (outPixel.x >= gAutoExposureOutputSize.x || outPixel.y >= gAutoExposureOutputSize.y)
		return;

	uint sx = lx * 2;
	uint sy = ly * 2;

	float avg = gSharedLuminance[sy + 0][sx + 0] + gSharedLuminance[sy + 0][sx + 1] 
			  + gSharedLuminance[sy + 1][sx + 0] + gSharedLuminance[sy + 1][sx + 1];

	avg *= 0.25f;

	gOutputLuminance[outPixel] = avg;
}

[numthreads(THREAD_X, THREAD_Y, 1)]
void CSReduceLuminance(
    uint3 nGroupID : SV_GroupID,
    uint3 nGroupThreadID : SV_GroupThreadID,
    uint3 nDispatchID : SV_DispatchThreadID)
{
	uint lx = nGroupThreadID.x;
	uint ly = nGroupThreadID.y;

	uint2 outPixel = nDispatchID.xy;

	int2 baseInput = int2(nGroupID.xy) * int2(TILE_X, TILE_Y);
	int2 localBase = int2(lx, ly) * 2;

    [unroll]
	for (uint oy = 0; oy < 2; ++oy)
	{
        [unroll]
		for (uint ox = 0; ox < 2; ++ox)
		{
			int2 local = localBase + int2(ox, oy);
			int2 src = baseInput + local;

			gSharedLuminance[local.y][local.x] = gInputLuminance.Load(int3(ClampPixel(src, gAutoExposureInputSize), 0));
		}
	}

	GroupMemoryBarrierWithGroupSync();

	if (outPixel.x >= gAutoExposureOutputSize.x || outPixel.y >= gAutoExposureOutputSize.y)
		return;

	uint sx = lx * 2;
	uint sy = ly * 2;

	float avg = gSharedLuminance[sy + 0][sx + 0] + gSharedLuminance[sy + 0][sx + 1] 
			  + gSharedLuminance[sy + 1][sx + 0] + gSharedLuminance[sy + 1][sx + 1];

	avg *= 0.25f;

	gOutputLuminance[outPixel] = avg;
}

#define THREAD_COUNT 256
groupshared float gShared[THREAD_COUNT];

[numthreads(THREAD_COUNT, 1, 1)]
void CSFinalLuminance(
    uint3 nGroupThreadID : SV_GroupThreadID)
{
	uint tid = nGroupThreadID.x;

	uint width = gAutoExposureInputSize.x;
	uint height = gAutoExposureInputSize.y;
	uint count = width * height;

	float sum = 0.0f;

	for (uint i = tid; i < count; i += THREAD_COUNT)
	{
		uint x = i % width;
		uint y = i / width;

		sum += gInputLuminance.Load(int3(x, y, 0));
	}

	gShared[tid] = sum;

	GroupMemoryBarrierWithGroupSync();

    [unroll]
	for (uint stride = THREAD_COUNT / 2; stride > 0; stride >>= 1)
	{
		if (tid < stride)
		{
			gShared[tid] += gShared[tid + stride];
		}

		GroupMemoryBarrierWithGroupSync();
	}

	if (tid == 0)
	{
		float avgLogLum = gShared[0] / max(float(count), 1.0f);
		gOutputLuminance[uint2(0, 0)] = avgLogLum;
	}
}
