#include <Windows.h>
#include <thread>
#include "memory/scanner.h"
#include "memory/resolver.h"

// Forwards from other modules
void Hooks_Install();
void Hooks_Remove();

namespace Cheat { bool g_Running=false, g_MenuOpen=false; }

DWORD WINAPI InitThread(LPVOID) {
    while (!GetModuleHandleW(L"gta-core-five.dll")) Sleep(500);

    g::GtaCore = scanner::GetModuleBase(L"gta-core-five.dll");
    g::GtaNet  = scanner::GetModuleBase(L"gta-net-five.dll");
    g::Script  = scanner::GetModuleBase(L"rage-scripting-five.dll");
    g::ScreenW = GetSystemMetrics(SM_CXSCREEN);
    g::ScreenH = GetSystemMetrics(SM_CYSCREEN);

    g::InitResolved();
    Hooks_Install();
    Cheat::g_Running = true;
    return 0;
}

BOOL APIENTRY DllMain(HMODULE h, DWORD r, LPVOID) {
    if (r == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(h);
        CreateThread(0, 0, InitThread, 0, 0, 0);
    }
    if (r == DLL_PROCESS_DETACH) {
        Hooks_Remove();
        Cheat::g_Running = false;
    }
    return TRUE;
}