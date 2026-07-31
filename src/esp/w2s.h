#pragma once
#include "../utils/vector.h"
#include "../memory/resolver.h"

// World-to-screen using in-memory view matrix
inline bool WorldToScreen(const Vec3& world, Vec2& out) {
    if (!g::ViewMatrixAddr) return false;
    auto& vm = *(const ViewMatrix*)g::ViewMatrixAddr;
    return vm.world_to_screen(world, out, (float)g::ScreenW, (float)g::ScreenH);
}