#pragma once

#define WIN32_LEAN_AND_MEAN             // 거의 사용되지 않는 내용을 Windows 헤더에서 제외합니다.
// Windows 헤더 파일
#include <windows.h>
#include <commctrl.h>	// Progress bar
#include "resource.h"	// Progress bar
#pragma comment(lib, "comctl32.lib")
// C 런타임 헤더 파일입니다.
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#undef min;
#undef max;

// STL
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

inline void DisplayText(const char* fmt, ...)
{
	va_list arg;
	va_start(arg, fmt);
	char cbuf[512 * 2];
	vsprintf_s(cbuf, fmt, arg);
	va_end(arg);

	int nLength = GetWindowTextLength(g_hMainEdit);
	SendMessage(g_hMainEdit, EM_SETSEL, nLength, nLength);
	SendMessageA(g_hMainEdit, EM_REPLACESEL, FALSE, (LPARAM)cbuf);
}

