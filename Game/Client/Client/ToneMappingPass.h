#pragma once
#include "RenderPass.h"

struct ToneMappingCommonParameters {
	float fExposure = 1.f;
	float fGamma = 2.2f;

	float fSaturation = 1.0f;
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

struct CB_TONE_MAPPING_DATA {
	float fExposure;
	float fGamma;
	float fSaturation;
	float fInputScale;

	float fOutputScale;
	float fGradingStrength;
	Vector2 pad;
};

class ToneMappingPass : public IRenderPass {
private:
	enum class TONE_MAPPING_MODE : uint32 { AGX = 0, ACES, UC2, GT, COUNT, UNDEFINED = std::numeric_limits<uint32>::max() };
	enum class DIRTY_UPDATE : uint32 { AGX = 0, ACES, UC2, GT, GRADING };


public:
	virtual void Initialize() override;

	virtual void Render(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		const RenderPassInput& input,
		OUT RenderPassOutput& output,
		OUT DescriptorHandle& outDescHandle) override;

	virtual void OnPreRender(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		const RenderPassInput& input, 
		OUT RenderPassOutput& output, 
		OUT DescriptorHandle& outDescHandle) override;


	virtual void OnPostRender(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		const RenderPassInput& input, 
		OUT RenderPassOutput& output, 
		OUT DescriptorHandle& outDescHandle) override;

	virtual void ShowDebugInfo() override;

private:
	void CreatePipelineState();
	void CreateRootSignature();

	CB_TONE_MAPPING_LUT_DATA MakeLUTCBData(DIRTY_UPDATE eDirty) const;
	CB_TONE_MAPPING_DATA MakeCBData() const;
	void SetDefaultParameters(TONE_MAPPING_MODE eModeBefore, TONE_MAPPING_MODE eModeAfter);

	void SaveParametersToJson() const;
	void SaveGrading() const;
	void SaveAgX() const;
	void SaveGT() const;
	void SaveUC2() const;
	void SaveACES() const;

	void LoadParametersFromJson();
	void LoadGrading();
	void LoadAgX();
	void LoadGT();
	void LoadUC2();
	void LoadACES();

	bool ShowDragFloat(
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

	bool ShowDragFloat3(
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

	ComPtr<ID3D12RootSignature> m_pd3dLUTRootSignature;
	ComPtr<ID3D12PipelineState> m_pd3dLUTBakingPipelineState[2];	// 0 : Tone map LUT / 1 : Look LUT

	TextureRef<UnorderedAccessTexture> m_ToneMapLUT;
	mutable bool m_bToneMapLUTDirtyFlags[4] = { true, true, true, true };	// All dirty

	TextureRef<UnorderedAccessTexture> m_GradingLUT;
	mutable bool m_bGradingLUTDirtyFlag = true;		// Dirty

	TONE_MAPPING_MODE m_eMode = TONE_MAPPING_MODE::ACES;

	// Parameters
	ToneMappingParameter m_Parameters;

	std::string m_strSaveName;

	// Debug messages
	const char* g_cstrModeName[4] = {
		"AgX", "ACES", "UC2", "GT"
	};

	mutable float m_fLastToneLUTUpdated = 0.f;
	mutable float m_fLastLookLUTUpdated = 0.f;

	inline const static std::string g_strSavePath = "../Resources/ToneMappings";
	const static uint32 g_unLUTSize = 32;
};
