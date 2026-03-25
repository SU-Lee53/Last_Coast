#include "NewCommon.hlsl"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tone Mapping

struct VS_QUAD_INPUT
{
	float3 position : POSITION;
	float2 uv : TEXCOORD0;
};

struct VS_QUAD_OUTPUT
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
};

VS_QUAD_OUTPUT VSToneMapping(VS_QUAD_INPUT input)
{
	VS_QUAD_OUTPUT output = (VS_QUAD_OUTPUT) 0;
	
	output.position = float4(input.position.xy, 0.f, 1.f);
	output.uv = input.uv;
	
	return output;
}

float3 GammaCorrect(float3 color, float fGamma)
{
	return pow(saturate(color), 1.0 / max(fGamma, 1e-6));
}

/////////////////////////////////////////////////////////////////////////////////
// ACES

float3 ACESFilm(float3 x)
{
	float a = 2.51;
	float b = 0.03;
	float c = 2.43;
	float d = 0.59;
	float e = 0.14;
	return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

/////////////////////////////////////////////////////////////////////////////////
// Agx

AgXParameters ExtractAgXParameters()
{
	AgXParameters ret;
	ret.fExposure = gToneMappingCommon0.x;
	ret.fGamma = gToneMappingCommon0.y;
	
	ret.fSaturation = gToneMappingData0.x;
	ret.fLookStrength = gToneMappingData0.y;
	ret.fInputScale = gToneMappingData0.z;
	ret.fOutputScale = gToneMappingData0.w;
	
	ret.slope = gToneMappingData1.xyz;
	ret.fContrastPivot = gToneMappingData1.w;
	
	ret.offset = gToneMappingData2.xyz;
	ret.fContrastStrength = gToneMappingData2.w;
	
	ret.power = gToneMappingData3.xyz;
	ret.fBlackLift = gToneMappingData3.w;
	
	ret.shadowTint = gToneMappingData4.xyz;
	ret.fShadowTintStrength = gToneMappingData4.w;
	
	ret.highlightTint = gToneMappingData5.xyz;
	ret.fHighlightTintStrength = gToneMappingData5.w;
	
	ret.fDensity = gToneMappingData6.x;
	ret.fLookSaturation = gToneMappingData6.y;
	
	ret.fShadowStartLuma= gToneMappingData6.z;
	ret.fShadowEndLuma= gToneMappingData6.w;
	ret.fHighlightStartLuma= gToneMappingData6.z;
	ret.fHighlightEndLuma = gToneMappingData6.w;
	
	return ret;
}

static const float3x3 gmtxAgXInsetMatrix =
{
	0.842479062253094, 0.0423282422610123, 0.0423756549057051,
    0.0784335999999992, 0.878468636469772, 0.0784336,
    0.0792237451477643, 0.0791661274605434, 0.879142973793104
};

static const float3x3 gmtxAgXOutsetMatrix =
{
	1.19687900512017, -0.0528968517574562, -0.0529716355144438,
    -0.0980208811401368, 1.15190312990417, -0.0980434501171241,
    -0.0990297440797205, -0.0989611768448433, 1.15107367264116
};

float3 AgxContrastApprox(float3 color)
{
	float3 color2 = color * color;
	float3 color4 = color2 * color2;
	
	return 15.5 * color4 * color2
	     - 40.14 * color4 * color
	     + 31.96 * color4
	     - 6.868 * color2 * color
	     + 0.4298 * color2
	     + 0.1191 * color
	     - 0.00232;
}

float GetLuminance(float3 color)
{
	return dot(color, float3(0.2126f, 0.7152f, 0.0722f));
}

float3 ApplySaturation(float3 color, float fSaturation)
{
	float luma = GetLuminance(color);
	return lerp(luma.xxx, color, fSaturation);
}

float3 ApplySlopeOffsetPower(float3 color, float3 slope, float3 offset, float3 power)
{
	color = color * slope + offset;
	color = max(color, 0.0f);
	color = pow(color, max(power, 1e-4f.xxx));
	return color;
}

float3 ApplyPivotContrast(float3 color, float fPivot, float fContrastStrength)
{
	return (color - fPivot) * fContrastStrength + fPivot;
}

float3 ApplyBlackLift(float3 color, float blackLift)
{
	float fLuma = GetLuminance(color);
	float fShadowMask = 1.0f - smoothstep(0.0f, 0.25f, fLuma);
	
	float fLiftedLuma = fLuma + blackLift * fShadowMask;
	float fScale = fLiftedLuma / max(fLuma, 1e-4f);
	
	fScale = min(fScale, 8.0f);
	
	return color * fScale;
}

float3 ApplyDensity(float3 color, float fDensity)
{
	float fLuma = GetLuminance(color);

	// Affects more when darker
	float weight = 1.0f - saturate(fLuma);

	// More shadow/midtone when density is greater
	float factor = max(1.0f - fDensity * weight, 0.0f);
	return color * factor;
}

float ComputeShadowMask(float fLuma, float fStartLuma, float fEndLuma)
{
	return 1.0f - smoothstep(fStartLuma, fEndLuma, fLuma);
}

float ComputeHighlightMask(float fLuma, float fStartLuma, float fEndLuma)
{
	return smoothstep(fStartLuma, fEndLuma, fLuma);
}

float3 ApplyShadowHighlightTint(float3 color, float3 shadowTint, float fShadowStrength, float2 shadowLuma, float3 highlightTint, float fHighlightStrength, float2 highlightLuma)
{
	float fLuma = GetLuminance(color);
	
	// Increase shadow mask when darker
	float shadowMask = ComputeShadowMask(fLuma, shadowLuma.x, shadowLuma.y);
	
	// Increase shadow mask when brighter
	float highlightMask = ComputeHighlightMask(fLuma, highlightLuma.x, highlightLuma.y);
	
	float3 shadowed = color * shadowTint;
	float3 highlighted = color * highlightTint;

	color = lerp(color, shadowed, saturate(shadowMask * fShadowStrength));
	color = lerp(color, highlighted, saturate(highlightMask * fHighlightStrength));

	return color;
}

float3 ApplyAgXLook(float3 color, AgXParameters params)
{
	// 1. Black lift
	color = ApplyBlackLift(color, params.fBlackLift);
	
	// 2. Base color shaping
	color = ApplySlopeOffsetPower(color, params.slope, params.offset, params.power);
	
	// 3. Contrast around pivot
	color = ApplyPivotContrast(color, params.fContrastPivot, params.fContrastStrength);
	
	// 4. Density
	color = ApplyDensity(color, params.fDensity);
	
	// 5. Shadow / highlight tint
	color = ApplyShadowHighlightTint(
	color, 
	params.shadowTint, params.fShadowTintStrength, float2(params.fShadowStartLuma, params.fShadowEndLuma),
	params.highlightTint, params.fHighlightTintStrength, float2(params.fHighlightStartLuma, params.fHighlightEndLuma));
	
	// 6. Look-local saturation
	color = ApplySaturation(color, params.fLookSaturation);
	
	return saturate(color);
}

float3 AgXCore(float3 hdrColor, AgXParameters params)
{
	// 1. Exposure
	float3 color = hdrColor * params.fExposure * params.fInputScale;
	
	// 2. White Control
	// Larger white allows wider brightness range → Highlight less quickly pressed
	// Not official AgX white control
	
	// 3. AgX inset transform
	color = mul(gmtxAgXInsetMatrix, max(color, 0.0f));
	
	// 4. Log2 encoding into working range
	const float fMinEv = -12.47393f;
	const float fMaxEv = 4.026069f;
	
	color = max(color, 1e-6f);
	color = (log2(color) - fMinEv) / (fMaxEv - fMinEv);
	color = saturate(color);

	// 5. Agx contrast curve approximation
	color = AgxContrastApprox(color);
	color = mul(gmtxAgXOutsetMatrix, color);
	
	return saturate(color);
}

float3 AgXToneMapping(float3 hdrColor)
{
	AgXParameters params = ExtractAgXParameters();
	
	float3 baseColor = AgXCore(hdrColor, params);
	float3 lookedColor = ApplyAgXLook(baseColor, params);
	
	float3 finalColor = lerp(baseColor, lookedColor, saturate(params.fLookStrength));
	finalColor = ApplySaturation(finalColor, params.fSaturation);
	finalColor *= params.fOutputScale;

	return saturate(finalColor);
}

//ACES Tone mapping
float4 PSToneMapping(VS_QUAD_OUTPUT input) : SV_Target0
{
	int2 pixelPos = int2(input.uv * gnScreenSize);
	pixelPos = clamp(pixelPos, int2(0, 0), gnScreenSize - 1);
	
	float3 hdr = gtxtHDRResult.Sample(gSamplerState, input.uv).rgb;
	
	//float3 mapped = ACESFilm(hdr);
	//mapped = pow(mapped, 1.0 / 2.2);
	float3 mapped = 0;
	
	switch (gnToneMappingType)
	{
	case 0:
		mapped = AgXToneMapping(hdr);
		break;
	
	case 1:
		mapped = AgXToneMapping(hdr);
		break;
	
	case 2:
		mapped = ACESFilm(hdr);
		break;
	}
	
	
	float fGamma = gToneMappingCommon0.y;
	mapped = GammaCorrect(mapped, fGamma);

	return float4(mapped, 1.0);
}
