#include "EffectCommon.hlsl"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Explosion

VS_PARTICLE_OUTPUT VSExplosion(VS_PARTICLE_INPUT input, uint nInstanceID : SV_InstanceID)
{
	int dataIndex = gnDataIndex + nInstanceID;
	float fElapsedTime = gParticleData[dataIndex].fElapsedTime;
	float3 vForce = gParticleData[dataIndex].vForce;
    
	float3 vNewPosition = input.position + gParticleData[dataIndex].vPosition;
	float4 cNewColor = float4(0.f, 0.f, 0.f, 0.f);
	float fNewAlpha = 0.f;
	float2 vNewSize = float2(0, 0);
    
	//float3 vNewPosition = input.position + gParticleData[dataIndex].vPosition;
	//float4 cNewColor = input.color;
	//float fNewAlpha = 1.f;
	//float2 vNewSize = input.initialSize;
    
	if (fElapsedTime <= input.lifeTime)
	{
		float fNewTime = fElapsedTime;
		float fNewTimeSq = fNewTime * fNewTime;
    	
		float fOneMinusTime = 1 - frac(fElapsedTime / input.lifeTime); // 1 ~ 0
    	
        // 새로운 위치의 계산
        
        // F = ma
        // 중력이 너무 약해서 좀 강하게 만듬
		float fForceX = vForce.x + (gvGravity.x * 35.f) * input.mass;
		float fForceY = vForce.y + (gvGravity.y * 35.f) * input.mass;
		float fForceZ = vForce.z + (gvGravity.z * 35.f) * input.mass;
    	
        // F = ma -> a = F / m
		float fAccX = fForceX / input.mass;
		float fAccY = fForceY / input.mass;
		float fAccZ = fForceZ / input.mass;
    	
        // s = v0t * 1/2at^2
		float fRandomValue = input.randomValue * 200.f;
		float3 initialDirection = normalize(input.initialVelocity);
		float dX = (initialDirection.x * fRandomValue * fNewTime) + (0.5 * fAccX * fNewTimeSq);
		float dY = (initialDirection.y * fRandomValue * fNewTime) + (0.5 * fAccY * fNewTimeSq);
		float dZ = (initialDirection.z * fRandomValue * fNewTime) + (0.5 * fAccZ * fNewTimeSq);
    	
		vNewPosition += float3(dX, dY, dZ);
    	
        // 알파값 계산 -> 시간이 지날수록 투명
        // 1 ~ 0 으로 변하는 값을 이용하여 잔해가 타서 점점 검어지는것도 표현 가능할 듯
		fNewAlpha = fOneMinusTime; // 1 ~ 0
		cNewColor.r = input.color.r * fOneMinusTime;
		cNewColor.g = input.color.g * fOneMinusTime;
		cNewColor.b = input.color.b * fOneMinusTime;
		cNewColor.a = fNewAlpha;
    	
        // 크기 계산 -> 시간이 지날수록 감소
		vNewSize = float2(input.initialSize.x * fOneMinusTime, input.initialSize.y * fOneMinusTime);
	}
	else
	{
        // lifeTime 이 지나면 저멀리 이상한곳으로 보냄
		vNewPosition += float3(9999999.f, 9999999.f, 9999999.f);
	}
    
	VS_PARTICLE_OUTPUT output;
	output.positionW = vNewPosition;
	output.color = cNewColor;
	output.size = vNewSize;
	output.nInstanceID = nInstanceID;
	return output;
}

[maxvertexcount(4)]
void GSExplosion(point VS_PARTICLE_OUTPUT input[1], inout TriangleStream<GS_PARTICLE_OUTPUT> outStream)
{
	float3 vUp = normalize(gmtxView._12_22_32);
	float3 vLook = gvCameraPosition.xyz - input[0].positionW;
	vLook = normalize(vLook);
	float3 vRight = cross(vUp, vLook);
    
	float4x4 mtxViewProjection = mul(gmtxView, gmtxProjection);
	
	int dataIndex = gnDataIndex + input[0].nInstanceID;
	//float3 pos = input[0].positionW + gParticleData[dataIndex].vPosition;
	
	float4 vertices[4];
	float2 vSize = input[0].size;
	vertices[0] = mul(float4(input[0].positionW + (vSize.x * 0.5 * vRight) + (vSize.x * 0.5 * vUp), 1.f), mtxViewProjection);
	vertices[1] = mul(float4(input[0].positionW - (vSize.x * 0.5 * vRight) + (vSize.x * 0.5 * vUp), 1.f), mtxViewProjection);
	vertices[2] = mul(float4(input[0].positionW + (vSize.x * 0.5 * vRight) - (vSize.x * 0.5 * vUp), 1.f), mtxViewProjection);
	vertices[3] = mul(float4(input[0].positionW - (vSize.x * 0.5 * vRight) - (vSize.x * 0.5 * vUp), 1.f), mtxViewProjection);
    
	float2 uvs[4] = { float2(0.f, 0.f), float2(1.f, 0.f), float2(0.f, 1.f), float2(1.f, 1.f) };
    
    // 출력
	GS_PARTICLE_OUTPUT output;
    
	output.position = vertices[0];
	output.color = input[0].color;
	output.uv = uvs[0];
	outStream.Append(output);
    
	output.position = vertices[1];
	output.color = input[0].color;
	output.uv = uvs[1];
	outStream.Append(output);
    
	output.position = vertices[2];
	output.color = input[0].color;
	output.uv = uvs[2];
	outStream.Append(output);
    
	output.position = vertices[3];
	output.color = input[0].color;
	output.uv = uvs[3];
	outStream.Append(output);
}

float4 PSExplosion(GS_PARTICLE_OUTPUT input) : SV_Target0
{
	return input.color;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Ray

VS_PARTICLE_OUTPUT VSRay(VS_PARTICLE_INPUT input, uint nInstanceID : SV_InstanceID)
{
	VS_PARTICLE_OUTPUT output;
	
	int dataIndex = gnDataIndex + nInstanceID;
	float fElapsedTime = gParticleData[dataIndex].fElapsedTime;
	float fTime = frac(fElapsedTime / input.lifeTime);
	float fFade = pow(1 - fTime, 2);
	
	output.positionW = input.position;
	output.color = float4(input.color.xyz, fFade);
	output.size = input.initialSize;
	output.nInstanceID = nInstanceID;

    return output;
}

[maxvertexcount(24)] // 6개의 면, 면당 정점 4개(strip)
void GSRay(point VS_PARTICLE_OUTPUT input[1], inout TriangleStream<GS_PARTICLE_OUTPUT> outStream)
{
	int dataIndex = gnDataIndex + input[0].nInstanceID;
	float3 vRayStartPosW = gParticleData[dataIndex].vPosition;
	float3 vRayDirectionW = normalize(gParticleData[dataIndex].vForce);
	
	if (all(vRayDirectionW == 0))
	{
		vRayDirectionW= float3(0, 0, 1);
	}
	
	float3 vUpRef = float3(0, 1, 0);
	if (abs(dot(vUpRef, vRayDirectionW) > 0.99f))
	{
		vUpRef = float3(1, 0, 0);
	}
	
	float3 vRight = normalize(cross(vUpRef, vRayDirectionW));
	float3 vUp = normalize(cross(vRayDirectionW, vRight));
	
	float fRayHalfWidth = 0.1f;
	float3 rx = vRight * fRayHalfWidth;
	float3 ry = vUp * fRayHalfWidth;

	float fRayLength = 10000.f;
	float3 p0 = vRayStartPosW;
	float3 p1 = vRayStartPosW + vRayDirectionW * fRayLength;
	
	float3 b00 = p0 - rx - ry;
	float3 b10 = p0 + rx - ry;
	float3 b11 = p0 + rx + ry;
	float3 b01 = p0 - rx + ry;

	float3 t00 = p1 - rx - ry;
	float3 t10 = p1 + rx - ry;
	float3 t11 = p1 + rx + ry;
	float3 t01 = p1 - rx + ry;
	
	float4x4 VP = mul(gmtxView, gmtxProjection);
	
	float3 vList[24] =
	{
		b01, b00, b11, b10,
        t00, t01, t10, t11,
        b11, b10, t11, t10,
        b00, b01, t00, t01,
        b11, b01, t11, t01,
        b00, b10, t00, t10
	};
	
	GS_PARTICLE_OUTPUT output;
	output.color = input[0].color;
	output.uv = float2(0, 0);
	
	[unroll]
	for (int i = 0; i < 24; i += 4)
	{
		output.position = mul(float4(vList[i + 0], 1), VP);
		outStream.Append(output);
		output.position = mul(float4(vList[i + 1], 1), VP);
		outStream.Append(output);
		output.position = mul(float4(vList[i + 2], 1), VP);
		outStream.Append(output);
		output.position = mul(float4(vList[i + 3], 1), VP);
		outStream.Append(output);
		outStream.RestartStrip();
	}
	
}

float4 PSRay(GS_PARTICLE_OUTPUT input) : SV_Target
{
    return input.color; // Red
}
