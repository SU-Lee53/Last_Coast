#include "Common.hlsl"

// 라인 디버그 렌더링용 셰이더
// POSITION만 사용하여 단색 라인 렌더링

struct VS_DEBUG_LINE_INPUT
{
    float3 position : POSITION;
};

struct VS_DEBUG_LINE_OUTPUT
{
    float4 position : SV_POSITION;
};

VS_DEBUG_LINE_OUTPUT VSDebugLine(VS_DEBUG_LINE_INPUT input)
{
    VS_DEBUG_LINE_OUTPUT output;

    // 월드 변환 없이 직접 뷰-프로젝션 적용
    // NavMesh 좌표는 이미 월드 좌표
    matrix mtxVP = mul(gmtxView, gmtxProjection);
    output.position = mul(float4(input.position, 1.0f), mtxVP);

    return output;
}

float4 PSDebugLineGreen(VS_DEBUG_LINE_OUTPUT input) : SV_TARGET
{
    // 녹색 라인 (폴리곤 외곽선)
    return float4(0.0f, 1.0f, 0.0f, 1.0f);
}

float4 PSDebugLineRed(VS_DEBUG_LINE_OUTPUT input) : SV_TARGET
{
    // 빨간색 라인 (인접성 그래프)
    return float4(1.0f, 0.0f, 0.0f, 1.0f);
}

float4 PSDebugLineBlue(VS_DEBUG_LINE_OUTPUT input) : SV_TARGET
{
    // 빨간색 라인 (인접성 그래프)
	return float4(0.0f, 0.0f, 1.0f, 1.0f);
}
