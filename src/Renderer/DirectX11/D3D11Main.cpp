//
// Created by Orgest on 7/4/2024.
//

#include "D3D11Main.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "dxguid.lib")

// D3D11Main::D3D11Main(Win32::WindowManager* winManager) winManager(winManager)

bool D3D11Main::Init()
{
	HRESULT hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&dData.dxgiFactory));
	if (FAILED(hr))
	{
		std::cout << "DXGI: Unable to create DXGIFactory\n";
		return false;
	}

	constexpr D3D_FEATURE_LEVEL deviceFeatureLevel = D3D_FEATURE_LEVEL_11_0;
	 hr = D3D11CreateDevice(
		nullptr,
		D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		0,
		&deviceFeatureLevel,
		1,
		D3D11_SDK_VERSION,
		&dData.device,
		nullptr,
		&dData.deviceContext);
	if (FAILED(hr))
	{
		std::cout << "D3D11: Failed to create device and device Context\n";
		return false;
	}

	// DXGI_SWAP_CHAIN_DESC1 swapChainDescriptor = {};
	// swapChainDescriptor.Width = winManager->;
	// swapChainDescriptor.Height = GetWindowHeight();
	// swapChainDescriptor.Format = DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM;
	// swapChainDescriptor.SampleDesc.Count = 1;
	// swapChainDescriptor.SampleDesc.Quality = 0;
	// swapChainDescriptor.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	// swapChainDescriptor.BufferCount = 2;
	// swapChainDescriptor.SwapEffect = DXGI_SWAP_EFFECT::DXGI_SWAP_EFFECT_FLIP_DISCARD;
	// swapChainDescriptor.Scaling = DXGI_SCALING::DXGI_SCALING_STRETCH;
	// swapChainDescriptor.Flags = {};
	//
	// return true;

	return true;
}

bool D3D11Main::Load()
{
	return true;
}

bool D3D11Main::CreateSwapchainResources()
{
	return true;
}

void D3D11Main::DestroySwapdhainResources()
{

}

void D3D11Main::OnResize(s32 width, s32 height)
{

}

void D3D11Main::Update()
{
}

void D3D11Main::Render()
{
}



