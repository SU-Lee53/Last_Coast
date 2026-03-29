#pragma once
#include "RenderPass.h"

struct ToneMappingCommonParameters {
	float fExposure = 1.f;
	float fGamma = 2.2f;

	float fSaturation = 1.0f;
	float fInputScale = 1.0f;	
	float fOutputScale = 1.0f;
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
	// TODO: Implement later
};

struct ACESParameters {
	// TODO: Implement later
};

struct LookParameters {
	Vector3 v3Slope;
	float fContrastPivot;

	Vector3 v3Offset;
	float fContrastStrength;

	Vector3 v3Power;
	float fBlackLift;

	Vector3 v3ShadowTint;
	float fShadowTintStrength;

	Vector3 v3HighlightTint;
	float fHighlightTintStrength;

	float fLookStrength;
	float fDensity;
	float fLookSaturation;
	float fShadowStartLuma;

	float fShadowEndLuma;
	float fHighlightStartLuma;
	float fHighlightEndLuma;
};

struct ToneMappingParameter {
	ToneMappingCommonParameters Common;
	LookParameters Look;

	union {
		AgXParameters AgX;
		GTParameters GT;
		UC2Parameters UC2;
		ACESParameters ACES;
	};

	ToneMappingParameter() {
		Common = g_DefaultCommonParameters;
		Look = g_DefaultLookParameters;
		AgX = g_DefaultAgXParameters;
	}

	constexpr static ToneMappingCommonParameters g_DefaultCommonParameters{
		.fExposure = 1.f,
		.fGamma = 2.2f,
		.fSaturation = 1.0f,
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

	constexpr static GTParameters g_DefaultGTParameters{
		.fGTMaxBrightness = 1.0f,
		.fGTContrast = 1.0f,
		.fGTLinearStart = 0.22f,
		.fGTLinearLength = 0.40f,
		.fGTBlack = 1.33f, 
		.fGTPedestal = 0.0f,
	};

	constexpr static LookParameters g_DefaultLookParameters{
		.v3Slope = Vector3(1.f, 1.f, 1.f),
		.fContrastPivot = 0.4f,
		.v3Offset = Vector3(0.0f, 0.0f, 0.0f),
		.fContrastStrength = 1.0f,
		.v3Power = Vector3(1.f, 1.f, 1.f),
		.fBlackLift = 0.02f,
		.v3ShadowTint = Vector3(1.f, 1.f, 1.f),
		.fShadowTintStrength = 0.1f,
		.v3HighlightTint = Vector3(1.f, 1.f, 1.f),
		.fHighlightTintStrength = 0.2f,
		.fDensity = 0.1f,
		.fLookSaturation = 1.0f,
		.fShadowStartLuma = 0.10f,
		.fShadowEndLuma = 0.55f,
		.fHighlightStartLuma = 0.45f,
		.fHighlightEndLuma = 0.85f,
	};
};

struct CB_TONE_MAPPING_DATA {
	uint32 nMode;
	Vector3 gToneMappingCommon0;	// x = exposure, y = gamma, z = saturation
	Vector4 gToneMappingCommon1;	// x = inputScale, y = outputScale, zw = reserved
	Vector4 gToneMappingCommon2;	// xyzw  = reserved
	
	// Common Look Parameters
	Vector3 v3Slope;
	float fContrastPivot;

	Vector3 v3Offset;
	float fContrastStrength;

	Vector3 v3Power;
	float fBlackLift;

	Vector3 v3ShadowTint;
	float fShadowTintStrength;

	Vector3 v3HighlightTint;
	float fHighlightTintStrength;

	float fLookStrength;
	float fDensity;
	float fLookSaturation;
	float fShadowStartLuma;

	float fShadowEndLuma;
	float fHighlightStartLuma;
	float fHighlightEndLuma;
};

class ToneMappingPass : public IRenderPass {
public:
	enum class TONE_MAPPING_MODE : uint32 {
		AGX = 0,
		GT = 1,
		UC2 = 2,
		ACES = 3,

		UNDEFINED = std::numeric_limits<uint32>::max()
	};

public:
	virtual void Initialize() override;

	virtual void Render(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		const RenderPassInput& input,
		OUT RenderPassOutput& output,
		OUT DescriptorHandle& outDescHandle) const override;

	virtual void OnPreRender(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		const RenderPassInput& input, 
		OUT RenderPassOutput& output, 
		OUT DescriptorHandle& outDescHandle) const override;


	virtual void OnPostRender(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		const RenderPassInput& input, 
		OUT RenderPassOutput& output, 
		OUT DescriptorHandle& outDescHandle) const override;

	virtual void ShowDebugInfo() override;

private:
	void CreatePipelineState();

	CB_TONE_MAPPING_DATA MakeCBData() const;
	void SetDefaultParameters(TONE_MAPPING_MODE eModeBefore, TONE_MAPPING_MODE eModeAfter);

	void SaveParametersToJson() const;
	void SaveLook() const;
	void SaveAgX() const;
	void SaveGT() const;
	void SaveACES() const;

	void LoadParametersFromJson();
	void LoadLook();
	void LoadAgX();
	void LoadGT();
	void LoadACES();

	void ShowDragFloat(
		int cnt,
		const char* cstrLabel, 
		float* v, 
		float fSpeed, 
		float fMin, 
		float fMax, 
		bool bShowHelp = true, 
		float fRecommandMin = 0.f, 
		float fRecommandMax = 0.f,
		float fDefault = 0.f,
		const char* cstrformat = "%.3f");

	void ShowDragFloat3(
		int cnt,
		const char* cstrLabel,
		float* v,
		float fSpeed,
		float fMin,
		float fMax,
		bool bShowHelp = true,
		float fRecommandMin = 0.f,
		float fRecommandMax = 0.f,
		float fDefault = 0.f);
private:
	ComPtr<ID3D12PipelineState> m_pd3dPipelineState;

	TONE_MAPPING_MODE m_eMode = TONE_MAPPING_MODE::AGX;

	// Common
	float m_fExposure = 1.f;
	float m_fGamma = 2.2f;

	// Parameters
	ToneMappingParameter m_Parameters;

	std::string m_strSaveName;

	// Debug messages
	const char* g_cstrModeName[4] = {
		"AgX", "GT", "UC2", "ACES"
	};


	inline const static std::string g_strSavePath = "../Resources/ToneMappings";
};
