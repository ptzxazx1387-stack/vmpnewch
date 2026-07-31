#pragma once
#include <cstdint>

namespace g {
    extern uintptr_t GtaCore, GtaNet, Script;
    extern uint64_t ViewMatrixAddr, CameraAddr, PlayerMgr, LocalPlayerPed;
    extern uintptr_t BoneOffset, PedPool, VehPool, ObjPool;
    extern int ScreenW, ScreenH;

    // FNV-1a 64 hash for pool name lookups
    uint64_t Fnv64(const char* str);

    // Initialize everything after module bases are set
    void InitResolved();

    // Call GetPoolBase(Hash("CPed")) etc.
    uint64_t GetPoolPtrFromHash(uint64_t hash);
}