#include "renderer.h"
#include "../vendor/imgui/imgui.h"
#include "../vendor/imgui/backends/imgui_impl_dx11.h"
#include "../vendor/imgui/backends/imgui_impl_win32.h"
#include "../proxy.h"

extern LRESULT CALLBACK WndProcHook(HWND, UINT, WPARAM, LPARAM);

namespace Renderer {

ID3D11Device* Device = nullptr;
ID3D11DeviceContext* Ctx = nullptr;
static bool g_Ready = false;

static WNDPROC g_OldWndProc = nullptr;

bool Init(ID3D11Device* dev, IDXGISwapChain*, HWND hwnd) {
    if (g_Ready) return true;
    Device = dev;
    dev->GetImmediateContext(&Ctx);
    if (!Ctx) return false;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

    g_OldWndProc = (WNDPROC)SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)WndProcHook);

    ImGui_ImplDX11_Init(Device, Ctx);
    ImGui_ImplWin32_Init(hwnd);

    ImGuiStyle& st = ImGui::GetStyle();
    st.Alpha = 0.82f;
    st.WindowRounding = 4.0f;
    ImGui::StyleColorsDark();

    g_Ready = true;
    return true;
}

void NewFrame() {
    if (!g_Ready) return;
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void EndFrame() {
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void Shutdown() {
    if (!g_Ready) return;
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    if (Ctx) Ctx->Release();
    g_Ready = false;
}

} // namespace Renderer

// Global wrappers for dx11_hook.cpp extern calls
void Render_Init(ID3D11Device* dev, IDXGISwapChain* sw, HWND hwnd) { Renderer::Init(dev, sw, hwnd); }
void Render_NewFrame() { Renderer::NewFrame(); }
void Render_Present()  { Renderer::EndFrame(); }
void Render_Shutdown() { Renderer::Shutdown(); }