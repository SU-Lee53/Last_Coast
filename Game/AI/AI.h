#pragma once

#ifdef AI_EXPORTS
#define AI_API __declspec(dllexport)
#else
#define AI_API __declspec(dllimport)
#endif

extern "C" AI_API int Add(int a, int b);
extern "C" AI_API void SayHello();