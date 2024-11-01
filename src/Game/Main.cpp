#include <iostream>

#include "../Platform/PlatformWindows.h"
#include "../Renderer/Vulkan/VulkanMain.h"
#include "Application.h"
#include "../Core/Vector.h"

#ifdef VULKAN_BUILD
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
#endif

int main()
{
	Logger::Init();

#ifdef VULKAN_BUILD
#ifdef DEBUG
	InitConsole();
#endif
#endif

	Application app;
	app.Run();

#ifdef DEBUG
	std::cin.get();
#endif
	return 0;
}



