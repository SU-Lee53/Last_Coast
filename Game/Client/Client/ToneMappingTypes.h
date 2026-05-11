#pragma once

struct ToneMappingCommonParameters {
	float fExposure = 1.f;
	float fGamma = 2.2f;

	float fPostSaturation = 1.0f;
	float fInputScale = 1.0f;
	float fOutputScale = 1.0f;
	float fGradingStrength = 0.f;
};

struct AgXParameters {
	float fAgXWhite = 1.0f;			// White ceiling / shoulder
	float fAgXBlack = 0.0f;			// Black floor / toe
	float fAgXContrast = 1.0f;		// AgX 곡선의 Contrast(대비)
	float fAgXMinEV = -12.47393f;
	float fAgXMaxEV = 4.026069f;	// 입력 HDR -> log working range
};

struct GTParameters {
	float fGTMaxBrightness = 1.0f;  // 최종 White/peak
	float fGTContrast = 1.0f;       // 커브 전체의 Contrast
	float fGTLinearStart = 0.22f;   // linear 시작점
	float fGTLinearLength = 0.40f;  // linear 길이
	float fGTBlack = 1.33f;         // toe/dkaqn shaping
	float fGTPedestal = 0.0f;       // black pedestal
};

struct UC2Parameters {
	float fUC2A;
	float fUC2B;
	float fUC2C;
	float fUC2D;

	float fUC2E;
	float fUC2F;
	float fUC2WhitePoint;
	float fUC2ExposureBias;
};

struct ACESParameters {
	float fACESExposureBias;
	float fACESPreSaturation;
	float fACESPostSaturation;
	float fACESHighlightDesaturation;
	float fACESCoreOutputScale;
};

struct GradingParameters {
	// White Balance(temterature, tint) + Primary
	Vector3 v3Slope;
	float fTemperature;
	Vector3 v3Offset;
	float fTint;
	Vector3 v3Power;
	float fColorFilterStrength; // This is originally Final parameter. Reordered cBuffer alignment.

	// Global
	float fContrast;
	float fContrastPivot;
	float fSaturation;
	float fDensity;

	// Tonal
	Vector3 v3ShadowTint;
	float  fShadowWeight;
	Vector3 v3MidtoneTint;
	float  fMidtoneWeight;
	Vector3 v3HighlightTint;
	float  fHighlightWeight;

	// Creative / Final
	Vector3 v3ColorFilter;
	float fBlackLift;
};

struct ToneMappingParameter {
	ToneMappingCommonParameters Common;
	GradingParameters Grading;

	union {
		AgXParameters AgX;
		GTParameters GT;
		UC2Parameters UC2;
		ACESParameters ACES;
	};

	ToneMappingParameter() {
		Common = g_DefaultCommonParameters;
		Grading = g_DefaultGradingParameters;
		AgX = g_DefaultAgXParameters;
	}

	constexpr static ToneMappingCommonParameters g_DefaultCommonParameters{
		.fExposure = 1.f,
		.fGamma = 2.2f,
		.fPostSaturation = 1.0f,
		.fInputScale = 1.0f,
		.fOutputScale = 1.0f,
	};

	constexpr static AgXParameters g_DefaultAgXParameters{
		.fAgXWhite = 1.0f,
		.fAgXBlack = 0.0f,
		.fAgXContrast = 1.0f,
		.fAgXMinEV = -12.47393f,
		.fAgXMaxEV = 4.026069f,
	};

	constexpr static ACESParameters g_DefaultACESParameters{
		.fACESExposureBias = 1.0f,
		.fACESPreSaturation = 1.0f,
		.fACESPostSaturation = 1.0f,
		.fACESHighlightDesaturation = 0.0f,
		.fACESCoreOutputScale = 1.0f,
	};

	constexpr static UC2Parameters g_DefaultUC2Parameters{
		.fUC2A = 0.15f,
		.fUC2B = 0.50f,
		.fUC2C = 0.10f,
		.fUC2D = 0.20f,
		.fUC2E = 0.02f,
		.fUC2F = 0.30f,
		.fUC2WhitePoint = 11.2f,
		.fUC2ExposureBias = 0.15f,
	};

	constexpr static GTParameters g_DefaultGTParameters{
		.fGTMaxBrightness = 1.0f,
		.fGTContrast = 1.0f,
		.fGTLinearStart = 0.22f,
		.fGTLinearLength = 0.40f,
		.fGTBlack = 1.33f,
		.fGTPedestal = 0.0f,
	};

	constexpr static GradingParameters g_DefaultGradingParameters{
		.v3Slope = Vector3(1.f),
		.fTemperature = 0.f,
		.v3Offset = Vector3(0.f),
		.fTint = 0.f,
		.v3Power = Vector3(1.f),
		.fColorFilterStrength = 0.f,
		.fContrast = 1.f,
		.fContrastPivot = 0.5f,
		.fSaturation = 1.f,
		.fDensity = 0.f,
		.v3ShadowTint = Vector3(1.f),
		.fShadowWeight = 0.f,
		.v3MidtoneTint = Vector3(1.f),
		.fMidtoneWeight = 0.f,
		.v3HighlightTint = Vector3(1.f),
		.fHighlightWeight = 0.f,
		.v3ColorFilter = Vector3(1.f),
		.fBlackLift = 0.f,
	};
};

struct CB_TONE_MAPPING_LUT_DATA {
	Vector4 gToneMappingCommon0;	// .x = nMode (reserved, Except grading LUT pass) 
	Vector4 gToneMappingCommon1;
	Vector4 gToneMappingCommon2;
	Vector4 gToneMappingCommon3;
	Vector4 gToneMappingCommon4;
	Vector4 gToneMappingCommon5;
	Vector4 gToneMappingCommon6;
	Vector4 gToneMappingCommon7;
};

struct CB_TONE_MAPPING_COMMON_DATA {
	float fExposure;
	float fGamma;
	float fSaturation;
	float fInputScale;

	float fOutputScale;
	float fGradingStrength;
	Vector2 pad;
};
