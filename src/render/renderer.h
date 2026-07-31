#pragma once
#include <Windows.h>
#include <d3d11.h>

struct IDXGISwapChain;

namespace Renderer {
    bool Init(ID3D11Device* device, IDXGISwapChain* swapChain, HWND window);
    void Shutdown();
    void NewFrame();
    void EndFrame();
    bool IsReady();
    extern ID3D11Device* Device;
    extern ID3D11DeviceContext* Ctx;
}