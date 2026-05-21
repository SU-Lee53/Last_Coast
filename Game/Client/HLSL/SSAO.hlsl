#include "NewCommon.hlsl"

static const int SSAO_KERNEL_SIZE = 16;

static const float3 gSSAOKernel[16] =
{
	float3(0.5381, 0.1856, 0.4319),
    float3(0.1379, 0.2486, 0.4430),
    float3(0.3371, 0.5679, 0.0057),
    float3(-0.6999, -0.0451, 0.0019),
    float3(0.0689, -0.1598, 0.8547),
    float3(0.0560, 0.0069, 0.1843),
    float3(-0.0146, 0.1402, 0.0762),
    float3(0.0100, -0.1924, 0.0344),
    float3(-0.3577, -0.5301, 0.4358),
    float3(-0.3169, 0.1063, 0.0158),
    float3(0.0103, -0.5869, 0.0046),
    float3(-0.0897, -0.4940, 0.3287),
    float3(0.7119, -0.0154, 0.0918),
    float3(-0.0533, 0.0596, 0.5411),
    float3(0.0352, -0.0631, 0.5460),
    float3(-0.4776, 0.2847, 0.0271)
};

float2 ProjectWorldToUV(float3 worldPos)
{
	matrix viewProj = mul(gCamera.mtxView, gCamera.mtxProjection);

	float4 clip = mul(float4(worldPos, 1.0f), viewProj);
	clip.xyz /= clip.w;

	float2 uv;
	uv.x = clip.x * 0.5f + 0.5f;
	uv.y = -clip.y * 0.5f + 0.5f;

	return uv;
}

float4 PSSSAO(VS_QUAD_OUTPUT input) : SV_Target0
{
	int2 pixelPos = int2(input.position.xy);
	float2 uv = input.position.xy / float2(gnScreenSize);

	GBufferData g = LoadGBuffer(pixelPos);

	if (g.depth >= 0.999999f)
	{
		return float4(1, 1, 1, 1);
	}

	float3 worldPos = ReconstructWorldPos(uv, g.depth);
	float3 normalW = normalize(g.normalW);

	float2 noiseUV = uv * (float2(gnScreenSize) / 4.0f) * gfSSAONoiseScale;
	float3 randomVec = gtxtSSAONoise.SampleLevel(gPointWrapSamplerState, noiseUV, 0.0f).xyz;
	randomVec = normalize(randomVec * 2.0f - 1.0f);

	float3 tangent = normalize(randomVec - normalW * dot(randomVec, normalW));
	float3 bitangent = normalize(cross(normalW, tangent));
	float3x3 TBN = float3x3(tangent, bitangent, normalW);

	float occlusion = 0.0f;
	int sampleCount = min(gnSSAOSampleCount, SSAO_KERNEL_SIZE);

    [loop]
	for (int i = 0; i < sampleCount; ++i)
	{
		float3 sampleDirW = mul(gSSAOKernel[i], TBN);
		float3 samplePosW = worldPos + sampleDirW * gfSSAORadius;

		float2 sampleUV = ProjectWorldToUV(samplePosW);

		if (sampleUV.x < 0.0f || sampleUV.x > 1.0f ||
            sampleUV.y < 0.0f || sampleUV.y > 1.0f)
		{
			continue;
		}

		float sampleDepth = gtxtGBufferDepth.SampleLevel(gSamplerState, sampleUV, 0.0f).r;

		if (sampleDepth >= 0.999999f)
		{
			continue;
		}

		float3 realSamplePosW = ReconstructWorldPos(sampleUV, sampleDepth);

		float3 toReal = realSamplePosW - worldPos;
		float dist = length(toReal);

		float rangeCheck = smoothstep(
            0.0f,
            1.0f,
            gfSSAORadius / max(dist, 1e-4f)
        );

		float expectedDist = length(samplePosW - worldPos);

		float blocked = dist < expectedDist - gfSSAOBias ? 1.0f : 0.0f;
		occlusion += blocked * rangeCheck;
	}

	float ao = 1.0f - occlusion / max(sampleCount, 1);
	ao = pow(saturate(ao), gfSSAOPower);
	ao = lerp(1.0f, ao, saturate(gfSSAOIntensity));

	return float4(ao, ao, ao, 1.0f);
}

static const int BLUR_RADIUS = 2;

float4 PSSSAOBilateralBlur(VS_QUAD_OUTPUT input) : SV_Target0
{
	int2 pixelPos = int2(input.position.xy);
	float2 uv = input.position.xy / float2(gnScreenSize);
	float2 texel = 1.0f / float2(gnScreenSize);

	GBufferData centerG = LoadGBuffer(pixelPos);
	float centerDepth = centerG.depth;
	float3 centerNormal = normalize(centerG.normalW);

	float sumAO = 0.0f;
	float sumWeight = 0.0f;

    [unroll]
	for (int y = -BLUR_RADIUS; y <= BLUR_RADIUS; ++y)
	{
        [unroll]
		for (int x = -BLUR_RADIUS; x <= BLUR_RADIUS; ++x)
		{
			float2 sampleUV = uv + float2(x, y) * texel;
			int2 samplePixel = pixelPos + int2(x, y);

			float ao = gtxtSSAOInput.SampleLevel(gSamplerState, sampleUV, 0.0f).r;

			GBufferData sampleG = LoadGBuffer(samplePixel);
			float depthDiff = abs(centerDepth - sampleG.depth);
			float depthWeight = exp(-depthDiff * gfSSAODepthSigma);

			float normalDot = saturate(dot(centerNormal, normalize(sampleG.normalW)));
			float normalWeight = pow(normalDot, gfSSAONormalSigma);

			float spatialWeight = exp(-float(x * x + y * y) / 8.0f);

			float weight = spatialWeight * depthWeight * normalWeight;

			sumAO += ao * weight;
			sumWeight += weight;
		}
	}

	float finalAO = sumAO / max(sumWeight, 1e-5f);

	return float4(finalAO, finalAO, finalAO, 1.0f);
}
