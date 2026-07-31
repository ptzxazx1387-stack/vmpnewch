#pragma once
#include <cstdint>

namespace g {
    extern uintptr_t GtaCore, GtaNet, Script;
    extern uint64_t ViewMatrixAddr, CameraAddr, PlayerMgr, LocalPlayerPed;
    extern uintptr_t BoneOffset, PedPool, VehPool, ObjPool;
    extern int ScreenW, ScreenH;
    inline uint64_t Fnv64(const char* str) {
        uint64_t h = 0xCBF29CE484222325ULL;
        while (*str) { h ^= (uint64_t)(*str++); h *= 0x100000001B3ULL; }
        return h;
    }
    void InitResolved();
}