#pragma once
#include "ToneMappingTypes.h"

enum class TONE_MAPPING_MODE : uint32 { 
	AGX = 0, 
	ACES, 
	UC2, 
	GT, 
	COUNT,
	UNDEFINED = std::numeric_limits<uint32>::max() 
};

enum LUT_DIRTY_FLAG : uint8 {
	TONE_MAPPER = 1,
	GRADING = TONE_MAPPER << 1,
};

class ToneMappingVolume {
private:
	inline const static std::string g_strSavePath = "../Resources/ToneMappings/";
	const char* m_cstrModeName[4] = {
		"AgX", "ACES", "UC2", "GT"
	};

public:
	ToneMappingVolume();
	void Update();

	void LoadFromFiles(const std::string& strFilename);

public:
	TONE_MAPPING_MODE GetCurrentToneMapper() const { return m_eCurrentToneMapper; }
	CB_TONE_MAPPING_COMMON_DATA GetCommonCBData() const;
	CB_TONE_MAPPING_LUT_DATA GetToneMapperLUTCBData() const;
	CB_TONE_MAPPING_LUT_DATA GetGradingLUTCBData() const;

	void SetDirtyFlag(uint8 eFlag, bool bValue);
	bool IsDirty(uint8 eFlag) const;

	// 화면 밝기 배율 (1.0 = 기본, <1 = 어둡게). 게임 이벤트 페이드용.
	void  SetOutputScale(float fScale) { m_Parameters.Common.fOutputScale = fScale; }
	float GetOutputScale() const { return m_Parameters.Common.fOutputScale; }

	// 톤매핑 Common 파라미터 직접 접근 (쓰기 가능). 작성형 게임 이벤트용.
	// exposure/gamma/saturation/outputScale/gradingStrength/autoExposure 등 라이브 반영.
	// (AgX/GT 등 톤커브 파라미터는 LUT 베이크 → 별도 DirtyFlag 필요, 여기선 미포함)
	ToneMappingCommonParameters& GetCommonParameters() { return m_Parameters.Common; }
	const ToneMappingCommonParameters& GetCommonParameters() const { return m_Parameters.Common; }

public:
	void ShowDebugOptions();

private:
	void SetDefaultParameters(TONE_MAPPING_MODE eModeBefore, TONE_MAPPING_MODE eModeAfter);

#pragma region SERIALIZATION
private:
	void SaveParametersToBinary(const std::string& strFilename) const;
	void LoadParametersFromBinary(const std::string& strFilename);

#pragma endregion 

#pragma region HELPER
private:
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
#pragma endregion 

private:
	// Parameters
	ToneMappingParameter m_Parameters;

	TONE_MAPPING_MODE m_eCurrentToneMapper = TONE_MAPPING_MODE::ACES;
	uint8 m_unDirtyFlag = LUT_DIRTY_FLAG::TONE_MAPPER | LUT_DIRTY_FLAG::GRADING;
	bool m_bSkipThisFrame = true;

private:
	// Save/Load
	std::string m_strSaveName;

};

