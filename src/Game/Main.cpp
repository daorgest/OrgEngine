#include <iostream>

#include "../Platform/PlatformWindows.h"
#include "../Renderer/Vulkan/VulkanMain.h"
#include "Application.h"
#include "../Core/Vector.h"

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
	Logger::Init();
 #ifdef DEBUG
     InitConsole();
 #endif

    Application app;
	// Vector<int> lol;
	//
	// lol.PushBack(1);
	// lol.PushBack(2);
	// lol.PushBack(3);
	// lol.PushBack(2);
	// lol.PushBack(3);
	// lol.PushBack(2);
	// lol.PushBack(3);
	// lol.PushBack(2);
	// lol.PushBack(3);
	// lol.PushBack(2);
	// lol.PushBack(3);
	// lol.PushBack(2);
	// lol.PushBack(3);
	//
	// for (int i = 0; i < lol.Size(); i++)
	// {
	// 	LOG(INFO, "Itemms[", i ,"] " ,lol[i]);
	// }

	app.Run();

	std::cin.get();
    return 0;
}
#endif


