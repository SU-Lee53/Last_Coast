#pragma once

#define WIN32_LEAN_AND_MEAN             // ���� ������ �ʴ� ������ Windows ������� �����մϴ�.
// Windows ��� ����
#include <windows.h>
#include <commctrl.h>	// Progress bar
#include "resource.h"	// Progress bar
#pragma comment(lib, "comctl32.lib")
// C ��Ÿ�� ��� �����Դϴ�.
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#undef min;
#undef max;

// STL
#include <cctype>
#include <iostream>
#include <fstream>
#include <print>
#include <memory>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <unordered_map>
#include <filesystem>

// Assimp
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/DefaultLogger.hpp>
#include <assimp/importerdesc.h>
#include <assimp/pbrmaterial.h>

// DirectXMaths
#include <DirectXMath.h>
#include <DirectXCollision.h>
using namespace DirectX;

// json
#include <nlohmann_json/json.hpp>
#include <nlohmann_json/json_fwd.hpp>

// DirectXTex
#include <DirectXTex/DirectXTex.h>
#include <DirectXTex/DirectXTex.inl>
#include <wincodec.h>

// lib
#if defined(_DEBUG)
#pragma comment(lib, "DirectXTex/Debug/DirectXTex.lib")
#pragma comment(lib, "Assimp/x64/assimp-vc143-mtd.lib")
#else
#pragma comment(lib, "DirectXTex/Release/DirectXTex.lib")
#pragma comment(lib, "Assimp/x64/assimp-vc143-mt.lib")
#endif



inline XMFLOAT4X4 aiMatrixToXMMatrix(const aiMatrix4x4& aiMat) {
	XMFLOAT4X4 xmf4x4Ret = {
		aiMat.a1, aiMat.b1, aiMat.c1, aiMat.d1,
		aiMat.a2, aiMat.b2, aiMat.c2, aiMat.d2,
		aiMat.a3, aiMat.b3, aiMat.c3, aiMat.d3,
		aiMat.a4, aiMat.b4, aiMat.c4, aiMat.d4,
	};	// Transposed

	return xmf4x4Ret;
}

inline XMFLOAT4 aiColorToXMVector(const aiColor4D& aiColor) {
	XMFLOAT4 xmf4Ret = {
		aiColor.r,
		aiColor.g,
		aiColor.b,
		aiColor.a,
	};

	return xmf4Ret;
}

inline XMFLOAT3 aiVector3DToXMVector(const aiVector3D& aiVector) {
	XMFLOAT3 xmf3Ret = {
		aiVector.x,
		aiVector.y,
		aiVector.z
	};

	return xmf3Ret;
}

inline XMFLOAT4 aiQuaternionToXMVector(const aiQuaternion& aiQuat) {
	XMFLOAT4 xmf4Ret = {
		aiQuat.x,
		aiQuat.y,
		aiQuat.z,
		aiQuat.w
	};

	return xmf4Ret;
}

inline std::wstring StringToWString_UTF8(const std::string& str)
{
	if (str.empty())
		return L"";

	int size_needed = MultiByteToWideChar(
		CP_UTF8,                // UTF-8
		0,
		str.c_str(),
		(int)str.size(),
		nullptr,
		0
	);

	std::wstring wstr(size_needed, 0);

	MultiByteToWideChar(
		CP_UTF8,
		0,
		str.c_str(),
		(int)str.size(),
		&wstr[0],
		size_needed
	);

	return wstr;
}

template<>
struct std::hash<aiString> {
	size_t operator()(const aiString& aiStr) const {
		return std::hash<std::string>{}(std::string{ aiStr.C_Str() });
	}
};


// Windows

inline HWND		g_hOpenButton;		// Open Button
inline HWND		g_hConvertButton;	// Convert Button
inline HWND		g_hMainEdit;		// Main Edit Control (Read Only)
inline HWND		g_hScaleEdit;		// Scale Factor Edit
inline HWND		g_hModelRadio;		// Model Radio
inline HWND		g_hAnimRadio;		// Animation Radio
inline HWND		g_hBakeOptionCheck;	

inline HWND		 g_hTextureCompressionCombo;
inline std::ofstream g_LogFile;
inline std::filesystem::path g_LogFilePath;

inline bool BeginConversionLog(const std::filesystem::path& saveDirectoryPath)
{
	if (g_LogFile.is_open()) {
		g_LogFile.close();
	}

	std::error_code ec;
	std::filesystem::create_directories(saveDirectoryPath, ec);
	g_LogFilePath = saveDirectoryPath / "AssimpConverterLog.txt";
	g_LogFile.open(g_LogFilePath, std::ios::out | std::ios::trunc);

	return g_LogFile.is_open();
}

inline void EndConversionLog()
{
	if (g_LogFile.is_open()) {
		g_LogFile.flush();
		g_LogFile.close();
	}
}

inline const std::filesystem::path& GetConversionLogPath()
{
	return g_LogFilePath;
}

inline void DisplayText(const char* fmt, ...)
{
	va_list arg;
	va_start(arg, fmt);
	char cbuf[4096];
	vsnprintf_s(cbuf, _countof(cbuf), _TRUNCATE, fmt, arg);
	va_end(arg);

	if (g_LogFile.is_open()) {
		g_LogFile << cbuf;
		g_LogFile.flush();
	}

	if (g_hMainEdit && IsWindow(g_hMainEdit)) {
		int nLength = GetWindowTextLength(g_hMainEdit);
		SendMessage(g_hMainEdit, EM_SETSEL, nLength, nLength);
		SendMessageA(g_hMainEdit, EM_REPLACESEL, FALSE, (LPARAM)cbuf);
	}
}
inline int ParseFilePaths(wchar_t* pwstrBuffer, std::vector<std::wstring>& outwstrFilePathes)
{
	wchar_t* p = pwstrBuffer;
	std::wstring directory = p;
	p += directory.length() + 1;

	if (*p == L'\0')
	{
		// ���� 1��
		outwstrFilePathes.push_back(directory);
	}
	else
	{
		// ���� ��
		while (*p)
		{
			std::wstring filename = p;
			outwstrFilePathes.push_back(directory + L"\\" + filename);
			p += filename.length() + 1;
		}
	}

	return outwstrFilePathes.size();
}

