//
// Created by Orgest on 4/12/2024.
//

#include "PlatformWindows.h"
namespace Win32
{

	WindowManager::WindowManager() : hInstance_(GetModuleHandle(nullptr)) {
		dimensions.screenWidth = GetSystemMetrics(SM_CXSCREEN) / 2;
		dimensions.screenHeight = GetSystemMetrics(SM_CYSCREEN) / 2;
	}


	WindowManager::~WindowManager()
	{
		windowStuff.running = false;
		// Clean up the created window
		if (hwnd_) {
			DestroyWindow(hwnd_);
			hwnd_ = nullptr;
		}
		UnregisterClass(appName_, hInstance_);
	}



	// handling events (like keypresses, rendering)
	LRESULT CALLBACK WindowManager::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
		LRESULT rez = 0;

		bool pressed;

		switch (msg)
		{
		case WM_CLOSE:
			windowStuff.running = false;
			break;
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYUP:
			pressed = (msg == WM_SYSKEYDOWN || msg == WM_KEYDOWN);
			for (int i = 0; i < Button::BUTTONS_COUNT; i++) {
				if (wp == Button::buttonValues[i]) {
					processEventButton(windowStuff.input.keyboard[i], pressed);
				}
			}
			return DefWindowProc(hwnd, msg, wp, lp);
		case WM_SETFOCUS:
			windowStuff.input.focused = true;
			break;

		case WM_KILLFOCUS:
			windowStuff.input.focused = false;
			break;
		default:
			//there are many messages that we didn't treat so we want to call the default window callback for those...
			rez = DefWindowProc(hwnd, msg, wp, lp);
		}

		return rez;
	}

	// This creates the window class and initiates window creation
	void WindowManager::CreateAppWindow() {
		WNDCLASSEX wcex
		{
			.cbSize = sizeof(wcex),
			.style = CS_HREDRAW | CS_VREDRAW,
			.lpfnWndProc = WndProc,
			.hInstance = hInstance_,
			.hCursor = LoadCursor(nullptr, IDC_ARROW),
			.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)), // YEAH A BLACK BG
			.lpszClassName = appName_,
		};

		if (!RegisterClassEx(&wcex))
		{
			MessageBox(nullptr, L"Cannot register window class :/.", L"Error", MB_OK | MB_ICONERROR);
			exit(1);
		}


		hwnd_ = CreateWindowEx(
			0,
			appName_,
			appName_,
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			dimensions.screenWidth,
			dimensions.screenHeight,
			nullptr,
			nullptr,
			hInstance_,
			this
		);


		if (!hwnd_) {
			MessageBox(nullptr, L"Failed to create window.", L"Error", MB_OK | MB_ICONERROR);
			exit(1); // Terminate if cannot create window
		}

		Logger::Log(INFO, "Win32 Initilzed ");

		// Now to show the window
		ShowWindow(hwnd_, SW_SHOW);
		SetForegroundWindow(hwnd_);
		SetFocus(hwnd_);
		UpdateWindow(hwnd_);
	}

	void WindowManager::InputHandler()
	{

		if (windowStuff.input.keyboard[Button::A].pressed)
		{
			std::cout << "Pressed!\n";
		}

		//std::cout << (int)windowStuff.input.keyBoard[Button::A].held << "\n";

		//std::cout << (int)windowStuff.input.focused << "\n";

		if (windowStuff.input.keyboard[Button::A].released)
		{
			std::cout << "Released!\n";
		}
		//
	}

	// Game / App loop
	void WindowManager::TheMessageLoop() const
	{
		MSG msg = {};


		while (windowStuff.running)
		{
			while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE) > 0) //remove all mesages from queue
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg); //call our window callback

			}


			InputHandler();

			// GAME LOOP GOES HERE

			if (!windowStuff.input.focused)
			{
				resetInput(windowStuff.input);
			}



			processInputAfter(windowStuff.input);
		}

	}
}

