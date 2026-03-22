#pragma once

#include "Types.h"
#include "CoreMacro.h"
#include "CoreTLS.h"
#include "CoreGlobal.h"
#include "Container.h"

#include "Windows.h"
#include <iostream>

#include <WinSock2.h>
#include <MSWSock.h>
#include <ws2tcpip.h>

#include "Lock.h"
#include "ObjectPool.h"
#include "TypeCast.h"
#include "Memory.h"

#pragma comment(lib, "ws2_32.lib")


void HelloWorld();
