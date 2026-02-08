#pragma once

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



// DirectXMath
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>


#include "SimpleMath.h"
using namespace DirectX::SimpleMath;

using namespace DirectX;
using namespace DirectX::PackedVector;




// Json
#include <nlohmann_json/json.hpp>
#include <nlohmann_json/json_fwd.hpp>

