//
// Created by Orgest on 4/11/2024.
//
#pragma once
#include    <iostream>
#include "Logger.h"



#ifdef _WIN32
#define		VK_USE_PLATFORM_WIN32_KHR
#define     NOMINMAX 1
#define     WIN32_LEAN_AND_MEAN
#define		WIN32_EXTRA_LEAN
#include    <Windows.h>
#endif



#ifdef DX12_Build
#include "d3dx12.h"
#include <dxgi1_6.h>
#endif


