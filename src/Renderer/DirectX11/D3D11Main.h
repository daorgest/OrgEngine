//
// Created by Orgest on 7/4/2024.
//

#ifndef D3D11MAIN_H
#define D3D11MAIN_H

#include <d3d11.h>
#include <dxgi1_6.h>
#include <wrl.h>

#include "../../Platform/PlatformWindows.h"
using Microsoft::WRL::ComPtr;

struct DX11Data
{
	ComPtr<IDXGIFactory2> dxgiFactory;
	ComPtr<ID3D11Device> device;
	ComPtr<ID3D11DeviceContext> deviceContext;
	ComPtr<IDXGISwapChain1> swapChain;
	ComPtr<ID3D11RenderTargetView> renderTarget;
};

class D3D11Main
{
public:
	explicit D3D11Main(Win32::WindowManager* winManager);
	~D3D11Main() = default;

	bool Init();
	bool Load();
	void OnResize(s32 width, s32 height);

	void Update();
	void Render();

private:
	DX11Data dData;

	Win32::WindowManager* winManager_;
	bool CreateSwapchainResources();
	void DestroySwapdhainResources();
};



#endif //D3D11MAIN_H
