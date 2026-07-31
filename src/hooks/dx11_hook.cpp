#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include "../proxy.h"

extern void Render_Init(ID3D11Device* dev, IDXGISwapChain* sw, HWND hwnd);
extern void Render_NewFrame();
extern void Render_Present();
extern void Render_Shutdown();
extern void Menu_Render();
extern void ESP_Run();
extern void Aimbot_Run();
extern void Game_Frame();

namespace DX11Hook {

typedef HRESULT(__stdcall* PresentFn)(IDXGISwapChain*, UINT, UINT);
static PresentFn g_OrigPresent = nullptr;
static bool g_Init = false;
static HWND g_Wnd = nullptr;

HRESULT __stdcall HookedPresent(IDXGISwapChain* pSwapChain, UINT sync, UINT flags) {
    if (!g_Init) {
        ID3D11Device* dev = nullptr;
        pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&dev);
        if (dev) {
            DXGI_SWAP_CHAIN_DESC desc = {};
            pSwapChain->GetDesc(&desc);
            g_Wnd = desc.OutputWindow;
            Render_Init(dev, pSwapChain, g_Wnd);
            g_Init = true;
            dev->Release();
        }
    }

    if (Cheat::g_Running) {
        Game_Frame();
        ESP_Run();
        Aimbot_Run();
    }

    if (g_Init) {
        Render_NewFrame();
        if (Cheat::g_MenuOpen) Menu_Render();
        Render_Present();
    }

    return g_OrigPresent(pSwapChain, sync, flags);
}

bool Install() {
    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"VMPCheat_Dummy";
    RegisterClassExW(&wc);
    HWND hwnd = CreateWindowW(L"VMPCheat_Dummy", L"", WS_OVERLAPPEDWINDOW,
        0, 0, 100, 100, nullptr, nullptr, wc.hInstance, nullptr);

    DXGI_SWAP_CHAIN_DESC sd = { 0 };
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 4;
    sd.BufferDesc.Height = 4;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL fl = D3D_FEATURE_LEVEL_11_0;
    IDXGISwapChain* sc = nullptr;
    ID3D11Device* d3d = nullptr;
    ID3D11DeviceContext* ctx = nullptr;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE,
        nullptr, 0, &fl, 1, D3D11_SDK_VERSION, &sd, &sc, &d3d, nullptr, &ctx);
    if (FAILED(hr)) {
        DestroyWindow(hwnd);
        UnregisterClassW(L"VMPCheat_Dummy", wc.hInstance);
        return false;
    }

    uintptr_t* vt = *(uintptr_t**)sc;
    g_OrigPresent = (PresentFn)vt[8];

    DWORD prot = 0;
    VirtualProtect(&vt[8], sizeof(uintptr_t), PAGE_EXECUTE_READWRITE, &prot);
    vt[8] = (uintptr_t)HookedPresent;

    sc->Release(); d3d->Release(); ctx->Release();
    DestroyWindow(hwnd);
    UnregisterClassW(L"VMPCheat_Dummy", wc.hInstance);
    return true;
}

void Remove() { g_Init = false; }
HWND GetGameWnd() { return g_Wnd; }

} // namespace DX11Hook