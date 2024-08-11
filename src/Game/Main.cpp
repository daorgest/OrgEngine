#include <iostream>
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

int main(int argc, char** argv)
{
#ifdef DEBUG
    Logger::Init();
    InitConsole();
#endif

    Platform::WindowContext windowContext;
    Platform::Win32 platform(&windowContext);
    GraphicsAPI::Vulkan::VkEngine engine(&windowContext);

    platform.Init();
    engine.Init();
    engine.Run();

    return 0;
}
#endif


