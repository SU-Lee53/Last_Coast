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

//////////////////////////////////////////////////////////////////////////////////
// 싱글톤 매크로 — 클라이언트 Defines.h 와 동일 패턴.
// 생성자는 각 클래스가 직접 private 으로 선언한다(멤버 초기화가 필요한 클래스 대응).

#define DECLARE_SINGLE(classname)					\
public:												\
	static classname* GetInstance()					\
	{												\
		static classname s_instance;				\
		return &s_instance;							\
	}												\
private:											\
	classname(const classname&) = delete;			\
	classname& operator=(const classname&) = delete;

#define GET_SINGLE(classname)	classname::GetInstance()

#define SHARED		GET_SINGLE(ServerShared)
#define GAMELOOP	GET_SINGLE(GameLoop)
#define NETWORK		GET_SINGLE(Network)
#define DB			GET_SINGLE(DBManager)
