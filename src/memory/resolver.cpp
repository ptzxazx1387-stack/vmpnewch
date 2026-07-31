#include <Windows.h>
#include <cstdio>
#include "resolver.h"
#include "scanner.h"
#include "offsets.h"

namespace g {

// Define static runtime variables
uintptr_t GtaCore = 0, GtaNet = 0, Script = 0;
uint64_t ViewMatrixAddr = 0, CameraAddr = 0, PlayerMgr = 0, LocalPlayerPed = 0;
uintptr_t BoneOffset = 0, PedPool = 0, VehPool = 0, ObjPool = 0;
int ScreenW = 1920, ScreenH = 1080;

// FNV-1a 64 for FiveM pool hashing
uint64_t Fnv64(const char* str) {
    uint64_t h = 0xCBF29CE484222325ULL;
    while (*str) { h ^= (uint64_t)(*str++); h *= 0x100000001B3ULL; }
    return h;
}

// Helper — call GetPoolBase from gta-core-five
uint64_t GetPoolPtrFromHash(uint64_t hash) {
    if (!GtaCore) return 0;
    typedef uint64_t(__fastcall* GetPoolBaseFn)(uint64_t);
    auto fn = (GetPoolBaseFn)(GtaCore + 0x9AFF0);
    return fn ? fn(hash) : 0;
}

void InitResolved() {
    if (!GtaCore) return;
    OutputDebugStringA("[VMP] InitResolved: starting resolution...\n");

    //=== Camera / ViewMatrix ================================================
    // Common GTAV FiveM pattern: mov rax, [g_ViewportGame]; test ebp, ebp;
    {
        size_t gtaSize = scanner::GetModuleSize(GtaCore);
        uint64_t vp_pattern = scanner::FindPattern(GtaCore, gtaSize,
            "48 8B 05 ?? ?? ?? ?? 48 8B D9 48 85 ED ?? ?? 8B 0D ?? ?? ?? ??");
        if (vp_pattern) {
            ViewMatrixAddr = scanner::GetRelativeAddr(vp_pattern, 3);
            // The resolved pointer points to a CViewportGame*.
            // View+Proj matrix is at +VIEWPORT_MATRIX inside the viewport.
            uint64_t viewport = *(uint64_t*)ViewMatrixAddr;
            if (viewport) {
                ViewMatrixAddr = viewport + offsets::VIEWPORT_MATRIX;
            }
        }

        // Fallback: direct known global pattern
        if (!ViewMatrixAddr || ViewMatrixAddr < 0x1000) {
            // Scan for cross-references to CViewportGame string
            uintptr_t alt = scanner::FindPattern(GtaCore, gtaSize,
                "48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B D8 48 85 C0 74");
            if (alt) {
                uint64_t viewport = *(uint64_t*)scanner::GetRelativeAddr(alt, 3);
                if (viewport && viewport > 0x1000) {
                    ViewMatrixAddr = viewport + offsets::VIEWPORT_MATRIX;
                }
            }
        }
    }

    //=== NetworkPlayerMgr ====================================================
    // String "netPlayerMgrBase" at gtaCore + 0x0E8DE8 is referenced by code
    // that loads the global singleton of CNetworkPlayerMgr
    if (GtaCore) {
        size_t gtaSize = scanner::GetModuleSize(GtaCore);

        // Pattern near "netPlayerMgrBase" string reference:
        // mov rax, [rip + ?] — one of the xrefs loads the Mgr address
        uintptr_t mgr_pat = scanner::FindPattern(GtaCore, gtaSize,
            "48 8B 05 ?? ?? ?? ?? 48 83 C1 08 FF 15 ?? ?? ?? ??");
        if (mgr_pat) {
            PlayerMgr = scanner::GetRelativeAddr(mgr_pat, 3);
        } else {
            // Fallback — search in gta-net-five.dll
            if (GtaNet) {
                size_t netSize = scanner::GetModuleSize(GtaNet);
                mgr_pat = scanner::FindPattern(GtaNet, netSize,
                    "48 8B 05 ?? ?? ?? ?? 48 8B D9 48 85 C0 74 ?? 48 8B 40 18");
                if (mgr_pat) {
                    PlayerMgr = scanner::GetRelativeAddr(mgr_pat, 3);
                }
            }
        }
    }

    //=== Camera pointer ======================================================
    // The game camera pointer is typically resolved from CGameCameraMgr
    // For now, we use a pattern near OnGameFrame init code
    if (GtaCore) {
        CameraAddr = *(uint64_t*)(GtaCore + 0x1462F0);
    }

    //=== Bone Offset =========================================================
    // CPed + 0x430 → crSkeleton (component that manages bone transforms)
    BoneOffset = offsets::PED_BONE_COMP;

    //=== Pool Addresses ======================================================
    // Direct: GetPoolBase(Fnv64("CPed")) returns base address of ped pool
    // Used indirectly via context.cpp → GetPoolFromHash()
    PedPool = GetPoolPtrFromHash(Fnv64("CPed"));
    VehPool = GetPoolPtrFromHash(Fnv64("CVehicle"));
    ObjPool = GetPoolPtrFromHash(Fnv64("CObject"));

    // Debug log
    char buf[512];
    sprintf_s(buf,
        "[VMP] Resolution done:\n"
        "  ViewMatrixAddr = 0x%llX\n"
        "  PlayerMgr      = 0x%llX\n"
        "  CameraAddr     = 0x%llX\n"
        "  BoneOffset     = 0x%llX\n"
        "  PedPool        = 0x%llX\n"
        "  VehPool        = 0x%llX\n",
        ViewMatrixAddr, PlayerMgr, CameraAddr, BoneOffset, PedPool, VehPool);
    OutputDebugStringA(buf);
}

} // namespace g