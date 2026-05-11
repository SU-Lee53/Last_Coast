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
	inline const static std::string g_strSavePath = "../Resources/ToneMappings";
	const char* m_cstrModeName[4] = {
		"AgX", "ACES", "UC2", "GT"
	};

public:
	ToneMappingVolume();
	void Update();

public:
	TONE_MAPPING_MODE GetCurrentToneMapper() const { return m_eCurrentToneMapper; }
	CB_TONE_MAPPING_COMMON_DATA GetCommonCBData() const;
	CB_TONE_MAPPING_LUT_DATA GetToneMapperLUTCBData() const;
	CB_TONE_MAPPING_LUT_DATA GetGradingLUTCBData() const;

	void SetDirtyFlag(uint8 eFlag, bool bValue);
	bool IsDirty(uint8 eFlag) const;

public:
	void ShowDebugOptions();

#pragma region SERIALIZATION
private:
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

	TONE_MAPPING_MODE m_eCurrentToneMapper;
	uint8 m_unDirtyFlag = LUT_DIRTY_FLAG::TONE_MAPPER | LUT_DIRTY_FLAG::GRADING;
	bool m_bSkipThisFrame = true;

private:
	// Save/Load
	std::string m_strSaveName;

};

