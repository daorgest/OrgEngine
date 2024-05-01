
//#include "../renderer/VulkanMain.h"

#include "../Platform/PlatformWindows.h"  // Include the header for your WindowManager class.
#include "../renderer/VulkanMain.h"

void enableVirtualTerminalProcessing() {
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD dwMode = 0;
	if (hOut == INVALID_HANDLE_VALUE || !GetConsoleMode(hOut, &dwMode)) return;
	SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	Win32::WindowManager windowManager;
	GraphicsAPI::VkEngine vk(windowManager);
	// Allocate a new console
	AllocConsole();

	// Attach standard input, output, and error to the console
	freopen_s(new FILE*, "CONIN$", "r", stdin);
	freopen_s(new FILE*, "CONOUT$", "w", stdout);
	freopen_s(new FILE*, "CONOUT$", "w", stderr);
	enableVirtualTerminalProcessing();

	std::cout << "Console Output Enabled\n";
	// Initialize the application instance

	windowManager.RegisterWindowClass();
	windowManager.CreateAppWindow();
	vk.InitVulkan();
	// Run the message loop
	windowManager.TheMessageLoop();

	std::cin.get();
	FreeConsole();

	return 0;
}
