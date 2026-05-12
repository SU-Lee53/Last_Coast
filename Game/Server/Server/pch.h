#pragma once

#include <iostream>
#include <string>
#include <random>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cmath>

#include <vector>
#include <array>
#include <unordered_map>

#include <thread>
#include <atomic>
#include <mutex>
#include <concurrent_unordered_map.h>
#include <concurrent_queue.h>

#include <memory>

#include <WS2tcpip.h>
#include <MSWSock.h>

#include "protocol.h"

#pragma comment(lib, "MSWSock.lib")
#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "winmm.lib")   // timeBeginPeriod

// DirectXMath (Vector3용 — header-only Windows SDK 라이브러리)
#undef min
#undef max
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include "SimpleMath.h"
using namespace DirectX::SimpleMath;
using namespace DirectX;

// nlohmann JSON — pch에 넣으면 ranges 충돌. 필요한 cpp에서 직접 include.

// AI DLL 인터페이스
#include "AI.h"
using namespace AIDLL;

#ifdef _DEBUG
#pragma comment(lib, "AI_dbg.lib")
#else
#pragma comment(lib, "AI.lib")
#endif
