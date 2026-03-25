#pragma once
#include "RenderPass.h"

struct AgXParameters
{
	float fSaturation = 1.0f;		// Data0.x
	float fLookStrength = 0.0f;		// Data0.y
	float fInputScale = 1.0f;		// Data0.z
	float fOutputScale = 1.0f;		// Data0.w

	// Look Parameters
	Vector3 v3Slope;				// Data1.xyz
	float fContrastPivot;			// Data1.w

	Vector3 v3Offset;				// Data2.xyz
	float fContrastStrength;		// Data2.w

	Vector3 v3Power;				// Data3.xyz
	float fBlackLift;				// Data3.w

	Vector3 v3ShadowTint;			// Data4.xyz
	float fShadowTintStrength;		// Data4.w

	Vector3 v3HighlightTint;		// Data5.xyz
	float fHighlightTintStrength;	// Data5.w

	float fDensity;					// Data6.x
	float fLookSaturation;			// Data6.y
	float fShadowStartLuma;			// Data6.z
	float fShadowEndLuma;			// Data6.w
	float fHighlightStartLuma;		// Data7.x
	float fHighlightEndLuma;		// Data7.y

};

struct UC2Parameters
{
	float fExposure;
	float fGamma;

	// TODO: Implement later
};

struct ACESParameters
{
	float fExposure;
	float fGamma;

	// TODO: Implement later
};

struct ToneMappingParameter {
	float fExposure = 1.f;
	float fGamma = 2.2f;

	union {
		AgXParameters AgX;
		UC2Parameters UC2;
		ACESParameters ACES;
	};

	ToneMappingParameter() {
		fExposure = 1.f;
		fGamma = 2.2f;
	}

};

struct CB_TONE_MAPPING_DATA {
	uint32 nMode;
	Vector3 gToneMappingCommon0;	// x = exposure, y = gamma, zw = reserved
	
	Vector4 gToneMappingData0;
	Vector4 gToneMappingData1;
	Vector4 gToneMappingData2;
	Vector4 gToneMappingData3;
	Vector4 gToneMappingData4;
	Vector4 gToneMappingData5;
	Vector4 gToneMappingData6;
	Vector4 gToneMappingData7;
};

class ToneMappingPass : public IRenderPass {
public:
	enum class TONE_MAPPING_MODE : uint32 {
		AGX = 0,
		UC2 = 1,
		ACES = 2,

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
	void SaveAgX() const;
	void SaveUC2() const;
	void SaveACES() const;

	void LoadParametersFromJson();
	void LoadAgX();
	void LoadUC2();
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
		float fDefault = 0.f);

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
	const char* g_cstrModeName[3] = {
		"AgX", "UC2", "ACES Filmic"
	};

	constexpr static AgXParameters g_AgxDefaultParameters{
		.fSaturation = 1.0f,
		.fLookStrength = 0.85f,
		.fInputScale = 1.0f,
		.fOutputScale = 1.0f,
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

	inline const static std::string g_strSavePath = "../Resources/ToneMappings";
};
