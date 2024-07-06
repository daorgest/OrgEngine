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
#include <thread>
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

// Export/Import Definitions
#ifdef ORGEXPORT
	// Exports
	#ifdef _MSC_VER
		#define ORGAPI __declspec(dllexport)
		#pragma comment(linker, "/subsystem:windows /ENTRY:mainCRTStartup")
	#else
		#define ORGAPI __attribute__((visibility("default")))
	#endif
#else
	// Imports
	#ifdef _MSC_VER
		#define ORGAPI __declspec(dllimport)
		#pragma comment(linker, "/subsystem:windows /ENTRY:mainCRTStartup")
	#else
		#define ORGAPI
	#endif
#endif

// Encapsulate DirectX includes in a single block
#ifdef DIRECTX_BUILD
#ifdef DX12_BUILD
#include <d3dx12.h>
#elif defined(DX11_BUILD)
#include <d3d11.h>
#endif
#include <dxgi1_6.h>
#include <wrl/client.h> // For Microsoft::WRL::ComPtr
using Microsoft::WRL::ComPtr;
#endif