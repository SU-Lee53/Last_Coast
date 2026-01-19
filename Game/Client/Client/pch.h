#pragma once
#pragma once

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // 거의 사용되지 않는 내용을 Windows 헤더에서 제외합니다.
// Windows 헤더 파일
#include <windows.h>
// C 런타임 헤더 파일입니다.
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#undef min
#undef max

// STL Essentials
#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <array>
#include <vector>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <deque>
#include <span>
#include <algorithm>
#include <type_traits>
#include <ranges>
#include <concepts>
#include <utility>
#include <filesystem>
#include <typeindex>


// Direct3D related headers
#include <wrl.h>
#include <shellapi.h>	

// D3D12
#include <d3d12.h>
#include <dxgi1_4.h>
#include <dxgidebug.h>
#include <d3dx/d3dx12.h>


// D3DCompiler
#include <d3dcompiler.h>

// DirectXMath
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>

// DirectXTex
#include <DirectXTex/DDSTextureLoader12.h>
#include <DirectXTex/WICTextureLoader12.h>

#include "SimpleMath.h"
using namespace DirectX::SimpleMath;

using namespace DirectX;
using namespace DirectX::PackedVector;

using namespace Microsoft::WRL;

// Import libraries
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")


// fmod sound library
#include <fmod/fmod.h>
#include <fmod/fmod.hpp>
#include <fmod/fmod_errors.h>
#ifdef _DEBUG
#pragma comment(lib, "fmod/fmodL_vc.lib")
#else
#pragma comment(lib, "fmod/fmod_vc.lib")
#endif

// Json
#include <nlohmann_json/json.hpp>
#include <nlohmann_json/json_fwd.hpp>


// WinSock
#include "WinSockCommon.h"

// Additional Helper Headers
#include "CommandListPool.h"
#include "Typedef.h"
#include "Defines.h"
#include "Concepts.h"
#include "Utility.h"
#include "ShaderResource.h"
#include "DescriptorHeap.h"
#include "Packets.h"
#include "RandomGenerator.h"
#include "AnimationHelper.h"

// ImGui
#include <ImGui/imgui.h>
#include <ImGui/imgui_impl_win32.h>
#include <ImGui/imgui_impl_dx12.h>

// Game Headers
#include "WinCore.h"
#include "D3DCore.h"
#include "GameFramework.h"

// Managers
#include "ResourceManager.h"
#include "RenderManager.h"
#include "ShaderManager.h"
#include "TextureManager.h"
#include "SceneManager.h"
#include "InputManager.h"
#include "GameTimer.h"
#include "ModelManager.h"
#include "GuiManager.h"
#include "NetworkManager.h"
#include "EffectManager.h"
#include "SoundManager.h"
#include "UIManager.h"
#include "AnimationManager.h"
