#include <Windows.h>
#include "../proxy.h"

namespace DX11Hook { bool Install(); void Remove(); }

static void CheckHotkeys() {
    static bool last = false;
    bool now = (GetAsyncKeyState(VK_INSERT) & 0x8000) != 0;
    if (now && !last) Cheat::g_MenuOpen = !Cheat::g_MenuOpen;
    last = now;
}

void Game_Frame() { CheckHotkeys(); }
void Hooks_Install() { DX11Hook::Install(); }
void Hooks_Remove()  { DX11Hook::Remove(); }