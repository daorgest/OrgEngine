#include <filesystem>


#include "../Platform/PlatformWindows.h"
#include "../Renderer/Vulkan/VulkanMain.h"

namespace fs = std::filesystem;

std::ofstream logFile;
std::streambuf* originalCoutBuf = nullptr;
std::streambuf* originalCerrBuf = nullptr;

#ifdef _WIN32
// Color coding for my logs
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
#endif
	Platform::WindowContext windowContext;
	windowContext.SetDimensions(1280, 720);
	Platform::Win32 platform(&windowContext);

	GraphicsAPI::Vulkan::VkEngine engine(&windowContext);

#ifdef DEBUG
	InitConsole();
#endif
	platform.Init();
	engine.Init();
	// Run the main loop
	engine.Run();

	std::cin.get();
	FreeConsole();

	return 0;
}
#endif