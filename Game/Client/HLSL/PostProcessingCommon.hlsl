#ifndef POST_PROCESSING_COMMON
#define POST_PROCESSING_COMMON

////////////////////////////////////////////////////////
//
//	- Space0 : Input/Output Texture resources + sampler
//	- space1 : buffer
//
////////////////////////////////////////////////////////


// space0 : Input, Output
Texture2D<float4> gInputTexture : register(t0, space0);
RWTexture2D<float4> gOutputTexture : register(u0, space0);

Texture2D<float> gInputLuminance : register(t1, space0);
RWTexture2D<float> gOutputLuminance : register(u1, space0);

SamplerState gLinearClampSampler : register(s0, space0);


// space1 : buffer
cbuffer cbBloomData : register(b0, space1)
{
	float gBloomThreshold;
	float gBloomSoftKnee;
	float gBloomIntensity;
	float gBloomRadius;

	int2 gBloomInputSize;
	int2 gBloomOutputSize;
}

cbuffer cbAutoExposureData : register(b1, space1)
{
	int2 gAutoExposureInputSize;
	int2 gAutoExposureOutputSize;
}

float ComputeLuminance(float3 c)
{
	return dot(c, float3(0.2126, 0.7152, 0.0722));
}

uint2 ClampPixel(int2 p, int2 size)
{
	return uint2(clamp(p, int2(0, 0), size - int2(1, 1)));
}



#endif
