
Texture2D<float4> gtxtInputHDRITexture : register(t0);
RWTexture2DArray<float4> gtxtOutputCubeMap : register(u0);

cbuffer cbCubeSize : register(b0)
{
	int gnWidth;
	int gnHeight;
};

SamplerState gSampler : register(s0);

#define PI 3.141592f

[numthreads(8,8,1)]
void CSHDRIToCubeMap(uint3 nDispatchID : SV_DispatchThreadID)
{
	uint face = nDispatchID.z;
	
	float2 uv;
	uv.x = (nDispatchID.x + 0.5f) / gnWidth;
	uv.y = (nDispatchID.y + 0.5f) / gnHeight;
	
	float2 p = uv * 2 - 1;
	//float2 p = float2(2 * uv.x - 1, 1 - 2 * uv.y);
	float3 dir = float3(0, 0, 0);
	
	switch (face)
	{
		case 0:
			dir = float3(1, p.y, -p.x); // +X
			break;
		case 1:
			dir = float3(-1, p.y, p.x); // -X
			break;
		case 2:
			dir = float3(p.x, -1, p.y); // -Y
			break;
		case 3:
			dir = float3(p.x, 1, -p.y); // +Y
			break;
		case 4:
			dir = float3(p.x, p.y, 1); // +Z
			break;
		case 5:
			dir = float3(-p.x, p.y, -1); // -Z
			break;
	}
	
	dir = normalize(dir);
	
	float phi = atan2(dir.z, dir.x);
	float theta = asin(dir.y);
	
	float2 hdrUV = float2(0, 0);
	hdrUV.x = phi / (2 * PI) + 0.5;
	hdrUV.y = theta / PI + 0.5;
	
	float4 color = gtxtInputHDRITexture.SampleLevel(gSampler, hdrUV, 0);
	
	gtxtOutputCubeMap[nDispatchID] = color;
}


