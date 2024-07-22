#include <iostream>
#include <windows.h>

#include "../Platform/PlatformWindows.h"
#include "../Renderer/Vulkan/VulkanMain.h"

#ifdef _WIN32
void InitConsole()
{
    // Allocate a new console
    AllocConsole();
    // Attach standard input, output, and error to the console
    freopen_s(new FILE*, "CONIN$", "r", stdin);
    freopen_s(new FILE*, "CONOUT$", "w", stdout);
    freopen_s(new FILE*, "CONOUT$", "w", stderr);

    std::cout << "Console Output Enabled\n";

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    if (hOut == INVALID_HANDLE_VALUE || !GetConsoleMode(hOut, &dwMode)) return;
    SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}



int main()
{
#ifdef DEBUG
    Logger::Init();
    InitConsole();
#endif

    Platform::WindowContext windowContext;
    windowContext.SetDimensions(1920, 1080);
    Platform::Win32 platform(&windowContext);
    GraphicsAPI::Vulkan::VkEngine engine(&windowContext);

    if (!platform.Init())
    {
        std::cerr << "Failed to initialize the platform.\n";
        return EXIT_FAILURE;
    }

    engine.Init();
    engine.Run();

    // Cleanup
    engine.Cleanup();
    platform.DestroyAppWindow();
    return 0;
}
#endif
