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

struct CommonParameters
{
	float fExposure;
	float fGamma;
	float fSaturation;
	float fInputScale;
	float fOutputScale;
};

struct AgXParameters
{
	float fAgXWhite;
	float fAgXBlack;
	float fAgXContrast;
	float fAgXMinEV;
	float fAgXMaxEV;
};

struct GTParameters
{
	float fGTMaxBrightness;
	float fGTContrast;
	float fGTLinearStart;
	float fGTLinearLength;
	float fGTBlack;
	float fGTPedestal;
};

struct UC2Parameters
{
	float fUC2A;
	float fUC2B;
	float fUC2C;
	float fUC2D;
	float fUC2E;
	float fUC2F;
	float fUC2WhitePoint;
	float fUC2ExposureBias;
};

struct ACESParameters
{
	float dummy;
};

struct LookParameters
{
	float3 v3Slope;
	float fContrastPivot;
	
	float3 v3Offset;
	float fContrastStrength;
	
	float3 v3Power;
	float fBlackLift;
	
	float3 v3ShadowTint;
	float fShadowTintStrength;
	
	float3 v3HighlightTint;
	float fHighlightTintStrength;
	
	float fLookStrength;
	float fDensity;
	float fLookSaturation;
	float fShadowStartLuma;
	
	float fShadowEndLuma;
	float fHighlightStartLuma;
	float fHighlightEndLuma;
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
// Parameter extract

CommonParameters ExtractCommonParameters()
{
	CommonParameters params;
	
	params.fExposure = gToneMappingCommon0.x;
	params.fGamma = gToneMappingCommon0.y;
	params.fSaturation = gToneMappingCommon0.z;
	params.fInputScale = gToneMappingCommon1.x;
	params.fOutputScale = gToneMappingCommon1.y;

	return params;
}

AgXParameters ExtractAgXParameters()
{
	AgXParameters params;
	
	params.fAgXWhite = gToneMappingCommon1.z;
	params.fAgXBlack = gToneMappingCommon1.w;
	params.fAgXContrast = gToneMappingCommon2.x;
	params.fAgXMinEV = gToneMappingCommon2.y;
	params.fAgXMaxEV = gToneMappingCommon2.z;
	
	return params;
}

GTParameters ExtractGTParameters()
{
	GTParameters params;
	
	params.fGTMaxBrightness = gToneMappingCommon1.z;
	params.fGTContrast = gToneMappingCommon1.w;
	params.fGTLinearStart = gToneMappingCommon2.x;
	params.fGTLinearLength = gToneMappingCommon2.y;
	params.fGTBlack = gToneMappingCommon2.z;
	params.fGTPedestal = gToneMappingCommon2.w;
	
	return params;
}

UC2Parameters ExtractUC2Parameters()
{
	UC2Parameters params;
	
	params.fUC2A = gToneMappingCommon1.z;
	params.fUC2B = gToneMappingCommon1.w;
	params.fUC2C = gToneMappingCommon2.x;
	params.fUC2D = gToneMappingCommon2.y;
	params.fUC2E = gToneMappingCommon2.z;
	params.fUC2F = gToneMappingCommon2.w;
	
	params.fUC2WhitePoint = gToneMappingCommon3.x;
	params.fUC2ExposureBias = gToneMappingCommon3.y;
	
	return params;
}

LookParameters ExtractLookParameters()
{
	LookParameters params;
	
	params.v3Slope = gv3Slope;
	params.fContrastPivot = gfContrastPivot;
	params.v3Offset = gv3Offset;
	params.fContrastStrength = gfContrastStrength;
	params.v3Power = gv3Power;
	params.fBlackLift = gfBlackLift;
	params.v3ShadowTint = gv3ShadowTint;
	params.fShadowTintStrength = gfShadowTintStrength;
	params.v3HighlightTint = gv3HighlightTint;
	params.fHighlightTintStrength = gfHighlightTintStrength;
	params.fLookStrength = gfLookStrength;
	params.fDensity = gfDensity;
	params.fLookSaturation = gfLookSaturation;
	params.fShadowStartLuma = gfShadowStartLuma;
	params.fShadowEndLuma = gfShadowEndLuma;
	params.fHighlightStartLuma = gfHighlightStartLuma;
	params.fHighlightEndLuma = gfHighlightEndLuma;

	return params;
}

/////////////////////////////////////////////////////////////////////////////////
// Look Transform

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

float3 ApplyLook(float3 color, LookParameters params)
{
	// 1. Black lift
	color = ApplyBlackLift(color, params.fBlackLift);
	
	// 2. Base color shaping
	color = ApplySlopeOffsetPower(color, params.v3Slope, params.v3Offset, params.v3Power);
	
	// 3. Contrast around pivot
	color = ApplyPivotContrast(color, params.fContrastPivot, params.fContrastStrength);
	
	// 4. Density
	color = ApplyDensity(color, params.fDensity);
	
	// 5. Shadow / highlight tint
	color = ApplyShadowHighlightTint(
	color, 
	params.v3ShadowTint, params.fShadowTintStrength, float2(params.fShadowStartLuma, params.fShadowEndLuma),
	params.v3HighlightTint, params.fHighlightTintStrength, float2(params.fHighlightStartLuma, params.fHighlightEndLuma));
	
	// 6. Look-local saturation
	color = ApplySaturation(color, params.fLookSaturation);
	
	return saturate(color);
}

/////////////////////////////////////////////////////////////////////////////////
// AgX

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

float3 AgXContrastApprox(float3 color)
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

float3 ApplyAgXContrastStrength(float3 color, float fContrast)
{
	const float3 pivot = 0.5f.xxx;
	return saturate((color - pivot) * fContrast + pivot);
}

float3 ApplyAgXEndpoints(float3 color, float fBlack, float fWhite)
{
	float fLow = saturate(fBlack);
	float fHigh = max(fWhite, fLow + 1e-4f);
	
	return saturate(fLow + color * (fHigh - fLow));
}

float3 AgXCore(float3 hdrColor, CommonParameters commonParams, AgXParameters params)
{
	// 1. Exposure / input scale
	float3 color = hdrColor * commonParams.fExposure * commonParams.fInputScale;
	
	// 2. Inset transform
	color = mul(gmtxAgXInsetMatrix, max(color, 0.f));
	
	// 3. Log2 encode into parameterized EV working range
	float fMinEV = params.fAgXMinEV;
	float fMaxEv = max(params.fAgXMaxEV, fMinEV + 1e-4f);
	
	color = max(color, 1e-6f);
	color = (log2(color) - fMinEV) / (fMaxEv - fMinEV);
	color = saturate(color);
	
	// 4. AgX base contrast curve
	color = AgXContrastApprox(color);
	
	// 5. Core contrast control
	color = ApplyAgXContrastStrength(color, params.fAgXContrast);
	
	// 6. Outset Transform
	color = mul(gmtxAgXOutsetMatrix, color);
	
	// 7. Core black / white endpoints
	color = ApplyAgXEndpoints(color, params.fAgXBlack, params.fAgXWhite);
	
	return saturate(color);
}

float3 AgXToneMapping(float3 hdrColor)
{
	CommonParameters commonParams = ExtractCommonParameters();
	AgXParameters agxParams = ExtractAgXParameters();
	LookParameters lookparams = ExtractLookParameters();
	
	float3 baseColor = AgXCore(hdrColor, commonParams, agxParams);
	float3 lookColor = ApplyLook(baseColor, lookparams);
	
	float3 finalColor = lerp(baseColor, lookColor, saturate(lookparams.fLookStrength));
	finalColor = ApplySaturation(finalColor, commonParams.fSaturation);
	finalColor *= commonParams.fOutputScale;
	
	return saturate(finalColor);
}

/////////////////////////////////////////////////////////////////////////////////
// Uchimura GT

float GTCurveScalar(float x, float P, float a, float m, float l, float c, float b)
{
	P = max(P, 1e-4f);
	a = max(a, 1e-4f);
	m = clamp(m, 1e-4f, P - 1e-4f);
	l = saturate(l);
	c = max(c, 1e-4f);
	b = max(b, 0.0f);

	float l0 = ((P - m) * l) / a;
	float S0 = m + l0;
	float S1 = m + a * l0;
	float C2 = (a * P) / max(P - S1, 1e-4f);
	float CP = -C2 / P;

	float w0 = 1.0f - smoothstep(0.0f, m, x);
	float w2 = step(m + l0, x);
	float w1 = 1.0f - w0 - w2;

	float T = m * pow(max(x, 0.0f) / max(m, 1e-4f), c) + b;
	float S = P - (P - S1) * exp(CP * (x - S0));
	float L = m + a * (x - m);

	return T * w0 + L * w1 + S * w2;
}

float3 GTCore(float3 hdrColor, CommonParameters commonParams, GTParameters params)
{
	float3 color = hdrColor * commonParams.fExposure * commonParams.fInputScale;
	color = max(color, 0.f);
	
	color.r = GTCurveScalar(color.r, params.fGTMaxBrightness, params.fGTContrast, params.fGTLinearStart, params.fGTLinearLength, params.fGTBlack, params.fGTPedestal);
	color.g = GTCurveScalar(color.g, params.fGTMaxBrightness, params.fGTContrast, params.fGTLinearStart, params.fGTLinearLength, params.fGTBlack, params.fGTPedestal);
	color.b = GTCurveScalar(color.b, params.fGTMaxBrightness, params.fGTContrast, params.fGTLinearStart, params.fGTLinearLength, params.fGTBlack, params.fGTPedestal);
	
	return saturate(color);
}

float3 GTToneMapping(float3 hdrColor)
{
	CommonParameters commonParams = ExtractCommonParameters();
	GTParameters GTParams = ExtractGTParameters();
	LookParameters lookparams = ExtractLookParameters();
	
	float3 baseColor = GTCore(hdrColor, commonParams, GTParams);
	float3 lookColor = ApplyLook(baseColor, lookparams);
	
	float3 finalColor = lerp(baseColor, lookColor, saturate(lookparams.fLookStrength));
	finalColor = ApplySaturation(finalColor, commonParams.fSaturation);
	finalColor *= commonParams.fOutputScale;
	
	return saturate(finalColor);
}

/////////////////////////////////////////////////////////////////////////////////
// Hable UC2

float UC2CurveScalar(float x, float A, float B, float C, float D, float E, float F)
{
	float fNumerator = x * (A * x + C * B) + D * E;
	float fDenominator = x * (A * x + B) + D * F;
	return (fNumerator / max(fDenominator, 1e-6f)) - (E / F);
}

float3 UC2CurveRGB(float3 x, float A, float B, float C, float D, float E, float F)
{
	float3 fNumerator = x * (A * x + C * B) + D * E;
	float3 fDenominator = x * (A * x + B) + D * F;
	return (fNumerator / max(fDenominator, 1e-6f.xxx)) - (E / F);
}

float3 UC2Core(float3 hdrColor, CommonParameters commonParams, UC2Parameters params)
{
	float3 color = hdrColor * commonParams.fExposure * commonParams.fInputScale;
	color = max(color, 0.f);
	color *= params.fUC2ExposureBias;
	
	color.r = UC2CurveScalar(color.r, params.fUC2A, params.fUC2B, params.fUC2C, params.fUC2D, params.fUC2E, params.fUC2F);
	color.g = UC2CurveScalar(color.g, params.fUC2A, params.fUC2B, params.fUC2C, params.fUC2D, params.fUC2E, params.fUC2F);
	color.b = UC2CurveScalar(color.b, params.fUC2A, params.fUC2B, params.fUC2C, params.fUC2D, params.fUC2E, params.fUC2F);
	float fWhiteScale = UC2CurveScalar(params.fUC2WhitePoint, params.fUC2A, params.fUC2B, params.fUC2C, params.fUC2D, params.fUC2E, params.fUC2F);
	
	color /= max(fWhiteScale, 1e-6f);
	
	return color;
}

float3 UC2ToneMapping(float3 hdrColor)
{
	CommonParameters commonParams = ExtractCommonParameters();
	UC2Parameters UC2Params = ExtractUC2Parameters();
	LookParameters lookparams = ExtractLookParameters();
	
	float3 baseColor = UC2Core(hdrColor, commonParams, UC2Params);
	float3 lookColor = ApplyLook(baseColor, lookparams);
	
	float3 finalColor = lerp(baseColor, lookColor, saturate(lookparams.fLookStrength));
	finalColor = ApplySaturation(finalColor, commonParams.fSaturation);
	finalColor *= commonParams.fOutputScale;
	
	return saturate(finalColor);
}

float4 PSToneMapping(VS_QUAD_OUTPUT input) : SV_Target0
{
	int2 pixelPos = int2(input.uv * gnScreenSize);
	pixelPos = clamp(pixelPos, int2(0, 0), gnScreenSize - 1);
	
	float3 hdr = gtxtHDRResult.Sample(gSamplerState, input.uv).rgb;
	
	float3 mapped = 0;
	switch (gnToneMappingType)
	{
	case 0:
		mapped = AgXToneMapping(hdr);
		break;
	
	case 1:
		mapped = GTToneMapping(hdr);
		break;
	
	case 2:
		mapped = UC2ToneMapping(hdr);
		break;
	
	case 3:
		mapped = ACESFilm(hdr);
		break;
	}
	
	
	float fGamma = gToneMappingCommon0.y;
	mapped = GammaCorrect(mapped, fGamma);

	return float4(mapped, 1.0);
}
