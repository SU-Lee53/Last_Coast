#pragma once

extern void ShowErrorMessage(std::string_view file, int line, std::string_view message);
#define SHOW_ERROR(strMsg)		ShowErrorMessage(__FILE__, __LINE__, strMsg);

// std::string -> std::wstring
inline std::wstring StringToWString(const std::string& str, UINT codePage = CP_UTF8)
{
    if (str.empty()) return L"";

    int size_needed = MultiByteToWideChar(codePage, 0, str.c_str(), -1, nullptr, 0);
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(codePage, 0, str.c_str(), -1, &wstr[0], size_needed);

    // 마지막에 들어간 null 문자 제거
    if (!wstr.empty() && wstr.back() == L'\0')
        wstr.pop_back();

    return wstr;
}

// std::wstring -> std::string
inline std::string WStringToString(const std::wstring& wstr, UINT codePage = CP_UTF8)
{
    if (wstr.empty()) return "";

    int size_needed = WideCharToMultiByte(codePage, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string str(size_needed, 0);
    WideCharToMultiByte(codePage, 0, wstr.c_str(), -1, &str[0], size_needed, nullptr, nullptr);

    if (!str.empty() && str.back() == '\0')
        str.pop_back();

    return str;
}

inline float SmoothStep(float fX, float fMin, float fMax)
{
	fX = std::clamp(fX, fMin, fMax);
	return fX * fX * (3.0f - 2.0f * fX);
}

inline float SmoothStep(double dX, double fMin, double fMax)
{
	dX = std::clamp(dX, fMin, fMax);
	return dX * dX * (3.0f - 2.0f * dX);
}

template<typename T>
struct ConstantBufferSize {
	constexpr static size_t value = (sizeof(T) + 255) & (~255);
    constexpr static size_t nDescriptors = ((sizeof(T) + 255) & (~255)) / 255;
};


/////////////////////////////////////////////////////////////////////////////////
// Unit Conversion + Literals

constexpr float CM(float v) noexcept { return v; }
constexpr float M(float v) noexcept { return v * 100.0f; }
constexpr float KM(float v) noexcept { return v * 100000.0f; }

constexpr float CM(int v) noexcept { return v; }
constexpr float M(int v) noexcept { return v * 100; }
constexpr float KM(int v) noexcept { return v * 100000; }

constexpr float operator""_cm(long double v) 
{
	return static_cast<float>(v);
}

constexpr float operator""_m(long double v) 
{
	return static_cast<float>(v * 100.0L);
}

constexpr float operator""_km(long double v) 
{
	return static_cast<float>(v * 100000.0L);
}

constexpr float operator""_cm(unsigned long long v) 
{
	return static_cast<float>(v);
}

constexpr float operator""_m(unsigned long long v)
{
	return static_cast<float>(v * 100ULL);
}

constexpr float operator""_km(unsigned long long v)
{
	return static_cast<float>(v * 100000ULL);
}
