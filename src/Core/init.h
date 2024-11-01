  #pragma once

// Standard Library Includes

// Project Includes
#include "Logger.h"
#include "PrimTypes.h"

// Platform-Specific Includes
#ifdef _WIN32
    #define NOMINMAX 1
#undef LoadImage

    #define WIN32_LEAN_AND_MEAN
    #include <Windows.h>
#endif

// ImGui Includes
#include "imgui.h"
#include "backends/imgui_impl_win32.h"

// Export/Import Definitions
#ifdef ORGEXPORT
    // Export Definition
    #ifdef _MSC_VER
        #define ORGAPI __declspec(dllexport)
        #pragma comment(linker, "/subsystem:windows /ENTRY:mainCRTStartup")
    #else
        #define ORGAPI __attribute__((visibility("default")))
    #endif
#else
    // Import Definition
    #ifdef _MSC_VER
        #define ORGAPI __declspec(dllimport)
        #pragma comment(linker, "/subsystem:windows /ENTRY:mainCRTStartup")
    #else
        #define ORGAPI
    #endif
#endif
