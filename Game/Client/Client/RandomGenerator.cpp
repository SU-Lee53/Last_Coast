#include "pch.h"
#include "RandomGenerator.h"

std::default_random_engine RandomGenerator::g_dre{ std::random_device{}() };
