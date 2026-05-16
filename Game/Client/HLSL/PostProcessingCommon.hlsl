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
SamplerState gLinearClampSampler : register(s0, space0);


// space1 : buffer
cbuffer cbBloomData : register(b0, space1)
{
	float gBloomThreshold;
	float gBloomSoftKnee;
	float gBloomIntensity;
	float gBloomRadius;

	int2 gInputSize;
	int2 gOutputSize;
}


#endif
