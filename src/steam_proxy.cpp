#include "proxy.h"

namespace SteamProxy {

static HMODULE g_Original = nullptr;

bool Init() {
    g_Original = LoadLibraryW(L"steam_api64_orig.dll");
    return g_Original != nullptr;
}

void Shutdown() {
    if (g_Original) { FreeLibrary(g_Original); g_Original = nullptr; }
}

HMODULE GetOriginal() { return g_Original; }

} // namespace SteamProxy