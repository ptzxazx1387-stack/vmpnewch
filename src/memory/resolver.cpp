#include <Windows.h>
#include "resolver.h"
#include "scanner.h"
#include "offsets.h"

namespace g {

// Define static runtime variables
uintptr_t GtaCore = 0, GtaNet = 0, Script = 0;
uint64_t ViewMatrixAddr = 0, CameraAddr = 0, PlayerMgr = 0, LocalPlayerPed = 0;
uintptr_t BoneOffset = 0, PedPool = 0, VehPool = 0, ObjPool = 0;
int ScreenW = 1920, ScreenH = 1080;

void InitResolved() {
    if (!GtaCore) return;

    // === Camera / ViewMatrix ===
    // Search for D3D11 viewport setup pattern
    // Pattern references the viewport constant buffer
    uint64_t vp_pattern = scanner::FindPattern(GtaCore, scanner::GetModuleSize(GtaCore),
        "48 8B 05 ?? ?? ?? ?? 48 8B D9 48 85 ED ?? ?? 8B 0D ?? ?? ?? ??");
    if (vp_pattern) {
        ViewMatrixAddr = scanner::GetRelativeAddr(vp_pattern, 3);
    }

    // === NetworkPlayerManager ===
    // Search for pattern in gta-net
    if (GtaNet) {
        uint64_t mgr_pat = scanner::FindModulePattern(L"gta-net-five.dll",
            "48 8B 05 ?? ?? ?? ?? 48 8B D9 48 85 C0 74 ?? 48 8B 40 18");
        if (mgr_pat) {
            PlayerMgr = scanner::GetRelativeAddr(mgr_pat, 3);
        }
    }

    // === Camera ===
    // Typical GTAV: camera at a global pointer
    if (GtaCore) {
        CameraAddr = *(uint64_t*)(GtaCore + 0x1462F0); // Resolve camera global
    }

    // === Pool addresses ===
    // Direct pools using GetPoolBase(fnv64("CPed")) etc.
    uint64_t ped = GtaCore + 0x9AFF0; // GetPoolBase
    uint64_t veh = GtaCore + 0x9B0A0; // GetPools
    // We'll populate pools at runtime via context calls, not offsets

    // === Bone offset ===
    // Trace from CPed to bone component (result from IDA)
    BoneOffset = 0x430;  // CPed + BoneComponent pointer

    // Debug resolution
    char b[256];
    sprintf(b, "Resolution[VMat = %p, PollMgr = %p, CameraAddr = %p]\n",
        (void*)ViewMatrixAddr, (void*)PlayerMgr, (void*)CameraAddr);
    OutputDebugStringA(b);
}

} // namespace g}