#include "../Platform/PlatformWindows.h"
#include "../Renderer/Vulkan/VulkanMain.h"


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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
#ifdef DEBUG
	Logger::Init();
#endif
	Win32::WindowManager windowManager;
	GraphicsAPI::VkEngine vulkanRenderer(&windowManager);

#ifdef DEBUG
	InitConsole();
#endif

	windowManager.RegisterWindowClass();
	windowManager.CreateAppWindow();
	vulkanRenderer.Init();
	// Run the main loop
	vulkanRenderer.Run();

	std::cin.get();
	FreeConsole();

	return 0;
}
#endif