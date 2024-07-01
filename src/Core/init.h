//
// Created by Orgest on 4/11/2024.
//
#pragma once

#include <deque>
#include <fstream>
#include <functional>
#include <iostream>
#include <span>
#include <string>
#include <vector>
#include "Logger.h"
#include "PrimTypes.h"


#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#define NOMINMAX 1
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <Windows.h>
#endif

#include "imgui.h"
#include "backends/imgui_impl_win32.h"

#ifdef DX12_BUILD
#include "d3dx12.h"
#include <dxgi1_6.h>
#endif


