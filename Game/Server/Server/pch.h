#pragma once

#include <iostream>
#include <WS2tcpip.h>
#include <MSWSock.h>

#ifdef _DEBUG
#pragma comment(lib, "Debug\\ServerCore.lib")
#pragma comment(lib, "AI_dbg.lib")
#else
#pragma comment(lib, "Release\\ServerCore.lib")
#pragma comment(lib, "AI.lib")
#endif

#include "CorePch.h"

// STL (CorePch.h 에 없는 것들)
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <random>

// DirectXMath (Vector3용 — header-only Windows SDK 라이브러리)
#undef min
#undef max
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include "SimpleMath.h"
using namespace DirectX::SimpleMath;
using namespace DirectX;

// AI DLL 인터페이스
#include "AI.h"
using namespace AIDLL;
#include <array>
#include <vector>

#include <thread>
#include <atomic>
#include <mutex>

#include "protocol.h"

#pragma comment(lib, "MSWSock.lib")
#pragma comment(lib, "WS2_32.lib")

