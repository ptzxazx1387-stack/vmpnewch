#pragma once
#include <Windows.h>

// Forward all steam_api64 exports to original DLL
namespace SteamProxy {
    bool Init();                          // Load original DLL, resolve all exports
    void Shutdown();
    const char* GetOriginalDllName();
    HMODULE GetOriginalModule();
}

// Main cheat initialization — call once gta-core-five is loaded
namespace Cheat {
    void Initialize();                    // Pattern scan, resolve offsets, hook
    void Shutdown();
    bool IsReady();

    extern bool g_Running;
    extern bool g_MenuOpen;
}