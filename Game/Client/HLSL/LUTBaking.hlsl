

// 1. Common Pre (Exposure * InputScale)
// 2. ToneMap LUT
// 3. Look LUT
// 4. lerp(ToneMapped, Looked, LookStrength)
// 5. Common Saturation
// 6. OutputScale
// 7. Gamma

cbuffer cbToneMappingData : register(b0, space0)
{
	float4 gToneMappingCommon0;	// x = nMode
	float4 gToneMappingCommon1;
	float4 gToneMappingCommon2;
	float4 gToneMappingCommon3;
	float4 gToneMappingCommon4;
	float4 gToneMappingCommon5;
	float4 gToneMappingCommon6;
};

RWTexture3D<float4> gtxtLUTToBuild : register(u0, space0);

const static uint gnLUTSize = 32;
const static float gfLUTMinEV = -10.0f;
const static float gfLUTMaxEV = 6.0f;

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
	float fACESExposureBias;
	float fACESPreSaturation;
	float fACESPostSaturation;
	float fACESHighlightDesaturation;
	float fACESCoreOutputScale;
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
	
	float fDensity;
	float fLookSaturation;
	float fShadowStartLuma;
	float fShadowEndLuma;
	
	float fHighlightStartLuma;
	float fHighlightEndLuma;
};

float GetLuminance(float3 color)
{
	return dot(color, float3(0.2126f, 0.7152f, 0.0722f));
}

float3 ApplySaturation(float3 color, float fSaturation)
{
	float luma = GetLuminance(color);
	return lerp(luma.xxx, color, fSaturation);
}

float3 LUTCoordToHDR(uint3 id)
{
	float3 uvw = (float3(id) + 0.5f) / float(gnLUTSize);
	float3 ev = lerp(gfLUTMinEV.xxx, gfLUTMaxEV.xxx, uvw);
	return exp2(ev);
}

/////////////////////////////////////////////////////////////////////////////////
// Parameter extract

AgXParameters ExtractAgXParameters()
{
	AgXParameters params;
	
	params.fAgXWhite = gToneMappingCommon0.y;
	params.fAgXBlack = gToneMappingCommon0.z;
	params.fAgXContrast = gToneMappingCommon0.w;
	params.fAgXMinEV = gToneMappingCommon1.x;
	params.fAgXMaxEV = gToneMappingCommon1.y;
	
	return params;
}

GTParameters ExtractGTParameters()
{
	GTParameters params;
	
	params.fGTMaxBrightness = gToneMappingCommon0.y;
	params.fGTContrast = gToneMappingCommon0.z;
	params.fGTLinearStart = gToneMappingCommon0.w;
	
	params.fGTLinearLength = gToneMappingCommon1.x;
	params.fGTBlack = gToneMappingCommon1.y;
	params.fGTPedestal = gToneMappingCommon1.z;
	
	return params;
}

UC2Parameters ExtractUC2Parameters()
{
	UC2Parameters params;
	
	params.fUC2A = gToneMappingCommon0.y;
	params.fUC2B = gToneMappingCommon0.z;
	params.fUC2C = gToneMappingCommon0.w;
	
	params.fUC2D = gToneMappingCommon1.x;
	params.fUC2E = gToneMappingCommon1.y;
	params.fUC2F = gToneMappingCommon1.z;
	params.fUC2WhitePoint = gToneMappingCommon1.w;
	
	params.fUC2ExposureBias = gToneMappingCommon2.x;
	
	return params;
}

ACESParameters ExtractACESParameters()
{
	ACESParameters params;
	
	params.fACESExposureBias = gToneMappingCommon0.y;
	params.fACESPreSaturation = gToneMappingCommon0.z;
	params.fACESPostSaturation = gToneMappingCommon0.w;
	
	params.fACESHighlightDesaturation = gToneMappingCommon1.x;
	params.fACESCoreOutputScale = gToneMappingCommon1.y;
	
	return params;
}

LookParameters ExtractLookParameters()
{
	LookParameters params;
	
	params.v3Slope = gToneMappingCommon0.yzw;
	
	params.fContrastPivot = gToneMappingCommon1.x;
	params.v3Offset = gToneMappingCommon1.yzw;
	
	params.fContrastStrength = gToneMappingCommon2.x;
	params.v3Power = gToneMappingCommon2.yzw;
	
	params.fBlackLift = gToneMappingCommon3.x;
	params.v3ShadowTint = gToneMappingCommon3.yzw;
	
	params.fShadowTintStrength = gToneMappingCommon4.x;
	params.v3HighlightTint = gToneMappingCommon4.yzw;
	
	params.fHighlightTintStrength = gToneMappingCommon5.x;
	params.fDensity = gToneMappingCommon5.y;
	params.fLookSaturation = gToneMappingCommon5.z;
	params.fShadowStartLuma = gToneMappingCommon5.w;
	
	params.fShadowEndLuma = gToneMappingCommon6.x;
	params.fHighlightStartLuma = gToneMappingCommon6.y;
	params.fHighlightEndLuma = gToneMappingCommon6.z;

	return params;
}

/////////////////////////////////////////////////////////////////////////////////
// Look Transform

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

float3 LUTCoordToUnitColor(uint3 id)
{
	return (float3(id) + 0.5f) / float(gnLUTSize);
}

float3 EvaluateLookLUT(float3 color)
{
	LookParameters lookParams = ExtractLookParameters();
	return ApplyLook(color, lookParams);
}

/////////////////////////////////////////////////////////////////////////////////
// ACES

// Krzysztof Narkowicz's simple ACES curve
float3 ACESSimple(float3 x)
{
	float a = 2.51;
	float b = 0.03;
	float c = 2.43;
	float d = 0.59;
	float e = 0.14;
	return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

// Stephen Hill / Baking Lab ACES fitted
static const float3x3 ACESInputMat =
{
	{ 0.59719, 0.35458, 0.04823 },
	{ 0.07600, 0.90834, 0.01566 },
	{ 0.02840, 0.13383, 0.83777 }
};

static const float3x3 ACESOutputMat =
{
	{ 1.60475, -0.53108, -0.07367 },
	{ -0.10208, 1.10813, -0.00605 },
	{ -0.00327, -0.07276, 1.07602 }
};

float3 RRTAndODTFit(float3 v)
{
	float3 a = v * (v + 0.0245786f) - 0.000090537f;
	float3 b = v * (0.983729f * v + 0.4329510f) + 0.238081f;
	return a / b;
}

float3 ACESFittedCore(float3 color)
{
	color = mul(ACESInputMat, color);
	color = RRTAndODTFit(color);
	color = mul(ACESOutputMat, color);
	return color;
}

float3 ApplyHighlightDesaturation(float3 color, float fAmount)
{
	float fLuma = GetLuminance(color);

	float t = saturate((fLuma - 1.0f) / 1.0f);
	t *= saturate(fAmount);
	
	float fGray = fLuma;
	return lerp(color, fGray.xxx, t);
}

float3 ACESCore(float3 hdrColor, ACESParameters params)
{
	float3 color = hdrColor;
	color = max(color, 0.f);
	
	// engine side pre-exposure
	color *= params.fACESExposureBias;
	
	// Pre-saturation
	color = ApplySaturation(color, params.fACESPreSaturation);
	
	// ACES Stype fit
	color = ACESFittedCore(color);
	
	// highlight desaturation
	color = ApplyHighlightDesaturation(color, params.fACESHighlightDesaturation);
	
	// post-saturation
	color = ApplySaturation(color, params.fACESPostSaturation);
	
	color *= params.fACESCoreOutputScale;
	
	return max(color, 0.f);
}

float3 EvaluateACESToneMapLUT(float3 hdrColor)
{
	ACESParameters acesParams = ExtractACESParameters();
	return ACESCore(hdrColor, acesParams);
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

float3 AgXCore(float3 hdrColor, AgXParameters params)
{
	// 1. Exposure / input scale
	float3 color = hdrColor;
	
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

float3 EvaluateAgXToneMapLUT(float3 hdrColor)
{
	AgXParameters agxParams = ExtractAgXParameters();
	return AgXCore(hdrColor, agxParams);
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

float3 GTCore(float3 hdrColor, GTParameters params)
{
	float3 color = max(hdrColor, 0.f);
	
	color.r = GTCurveScalar(color.r, params.fGTMaxBrightness, params.fGTContrast, params.fGTLinearStart, params.fGTLinearLength, params.fGTBlack, params.fGTPedestal);
	color.g = GTCurveScalar(color.g, params.fGTMaxBrightness, params.fGTContrast, params.fGTLinearStart, params.fGTLinearLength, params.fGTBlack, params.fGTPedestal);
	color.b = GTCurveScalar(color.b, params.fGTMaxBrightness, params.fGTContrast, params.fGTLinearStart, params.fGTLinearLength, params.fGTBlack, params.fGTPedestal);
	
	return saturate(color);
}

float3 EvaluateGTToneMapLUT(float3 hdrColor)
{
	GTParameters GTParams = ExtractGTParameters();
	
	return GTCore(hdrColor, GTParams);
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

float3 UC2Core(float3 hdrColor, UC2Parameters params)
{
	float3 color = max(hdrColor, 0.f);
	color *= params.fUC2ExposureBias;
	
	color.r = UC2CurveScalar(color.r, params.fUC2A, params.fUC2B, params.fUC2C, params.fUC2D, params.fUC2E, params.fUC2F);
	color.g = UC2CurveScalar(color.g, params.fUC2A, params.fUC2B, params.fUC2C, params.fUC2D, params.fUC2E, params.fUC2F);
	color.b = UC2CurveScalar(color.b, params.fUC2A, params.fUC2B, params.fUC2C, params.fUC2D, params.fUC2E, params.fUC2F);
	float fWhiteScale = UC2CurveScalar(params.fUC2WhitePoint, params.fUC2A, params.fUC2B, params.fUC2C, params.fUC2D, params.fUC2E, params.fUC2F);
	
	color /= max(fWhiteScale, 1e-6f);
	
	return color;
}

float3 EvaluateUC2ToneMapLUT(float3 hdrColor)
{
	UC2Parameters UC2Params = ExtractUC2Parameters();
	return UC2Core(hdrColor, UC2Params);
}

[numthreads(4, 4, 4)]
void CSToneMapLUT(uint3 nDispatchID : SV_DispatchThreadID)
{
	if (nDispatchID.x >= gnLUTSize || nDispatchID.y >= gnLUTSize || nDispatchID.z >= gnLUTSize)
	{
		return;
	}
	
	uint nMode = (uint)gToneMappingCommon0.x;
	
	float3 hdrColor = LUTCoordToHDR(nDispatchID);
	float3 baked = 0;
	
	switch (nMode)
	{
		case 0:
			baked = EvaluateAgXToneMapLUT(hdrColor);
			break;
		
		case 1:
			baked = EvaluateACESToneMapLUT(hdrColor);
			break;
	
		case 2:
			baked = EvaluateUC2ToneMapLUT(hdrColor);
			break;
	
		case 3:
			baked = EvaluateGTToneMapLUT(hdrColor);
			break;
	}
	
	gtxtLUTToBuild[nDispatchID] = float4(baked, 1.0f);
}

[numthreads(4, 4, 4)]
void CSLookLUT(uint3 nDispatchID : SV_DispatchThreadID)
{
	if (nDispatchID.x >= gnLUTSize || nDispatchID.y >= gnLUTSize || nDispatchID.z >= gnLUTSize)
	{
		return;
	}
	
	float3 color = LUTCoordToUnitColor(nDispatchID);
	float3 baked = EvaluateLookLUT(color);
	
	gtxtLUTToBuild[nDispatchID] = float4(baked, 1.0f);
}
