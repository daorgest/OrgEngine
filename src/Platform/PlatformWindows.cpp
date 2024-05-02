//
// Created by Orgest on 4/12/2024.
//

#include "PlatformWindows.h"

namespace Win32
{
	// Definition of the static member variable
	Input WindowManager::input;

	WindowManager::WindowManager() : hwnd_(nullptr), hInstance_(GetModuleHandle(nullptr))
	{
		dimensions.screenWidth = GetSystemMetrics(SM_CXSCREEN) / 2;
		dimensions.screenHeight = GetSystemMetrics(SM_CYSCREEN) / 2;
	}


	WindowManager::~WindowManager()
	{
		if (hwnd_) {
			if (!DestroyWindow(hwnd_)) {
				LOG(ERR, "Failed to destroy window. HWND:", hwnd_);
			}
			hwnd_ = nullptr;
		}
		UnregisterClass(appName_, hInstance_);
	}



	// handling events (like keypresses, rendering)
	LRESULT CALLBACK WindowManager::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
		LRESULT rez = 0;

		switch (msg)
		{
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
			if (wp < Button::BUTTONS_COUNT) {
				processEventButton(input.keyboard[wp], true);  // Key NOT pressed
			}
			break;
		case WM_SYSKEYUP:
		case WM_KEYUP:
			if (wp < Button::BUTTONS_COUNT) {
				processEventButton(input.keyboard[wp], false);  // Key NOT pressed
			}
			break;
		case WM_MOUSEMOVE:
			// Update mouse position
			input.cursorX = LOWORD(lp);
			input.cursorY = HIWORD(lp);
			break;
		case WM_LBUTTONDOWN:
			processEventButton(input.lMouseButton, true);
			break;

		case WM_LBUTTONUP:
			processEventButton(input.lMouseButton, false);
			break;

		case WM_RBUTTONDOWN:
			processEventButton(input.rMouseButton, true);
			break;

		case WM_RBUTTONUP:
			processEventButton(input.rMouseButton, false);
			break;

		case WM_SETFOCUS:
			LOG(INFO, "Window in focus");
			input.focused = true;
			break;

		case WM_KILLFOCUS:
			input.focused = false;
			resetInput(input);
			break;
		case WM_CLOSE:
			windowStuff.running = false;
			PostQuitMessage(0);
			break;
		default:
			//there are many messages that we didn't treat so we want to call the default window callback for those...
			rez = DefWindowProc(hwnd, msg, wp, lp);
		}

		return rez;
	}

	// This creates the window class and initiates window creation
	bool WindowManager::RegisterWindowClass() const
	{
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
		return true;
	}

	bool WindowManager::CreateAppWindow()
	{
		hwnd_ = CreateWindowEx(
			0,
			appName_,
#ifdef DEBUG
L"OrgEngine - Debug",            // Window text (Debug version)
#else
L"OrgEngine - Release",          // Window text (Release version)
#endif
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			dimensions.screenWidth,
			dimensions.screenHeight,
			nullptr,
			nullptr,
			hInstance_,
			nullptr
		);


		if (!hwnd_) {
			MessageBox(nullptr, L"Failed to create window.", L"Error", MB_OK | MB_ICONERROR);
			return false;
		}

		// Now to show the window
		ShowWindow(hwnd_, SW_SHOW);
		SetForegroundWindow(hwnd_);
		SetFocus(hwnd_);
		UpdateWindow(hwnd_);

		LOG(INFO, "Window created successfully. HWND:", hwnd_);
		return true;
	}
	// Definition of the LogInputStates method
	void WindowManager::LogInputStates() const
	{

		if (input.keyboard[Button::A].pressed)
		{
			LOG(INFO, "A has been pressed");
		}
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
			LogInputStates();

			// GAME LOOP GOES HERE

			if (input.focused)
			{
				resetInput(input);
			}

			processInputAfter(input);
		}
	}
}

