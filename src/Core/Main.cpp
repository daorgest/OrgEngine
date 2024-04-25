
//#include "../renderer/VulkanMain.h"


#include "../Platform/PlatformWindows.h"  // Include the header for your WindowManager class.
#include "../renderer/VulkanMain.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	// Initialize the application instance
	Win32::WindowManager windowManager;

	GraphicsAPI::Vulkan vk;

	windowManager.CreateAppWindow();
	std::cout << "hWnd: " << windowManager.getHwnd() << std::endl;
	vk.InitVulkanAPI(windowManager);
	// Run the message loop
	windowManager.TheMessageLoop();

	return 0;
}
