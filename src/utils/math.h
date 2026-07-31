#pragma once
#include <algorithm>
#include <cmath>

namespace math {
    template<typename T> T clamp(T v, T lo, T hi) { return std::min(std::max(v, lo), hi); }
    inline float lerp(float a, float b, float t) { return a + (b - a) * clamp(t, 0.0f, 1.0f); }
    inline float deg2rad(float d) { return d * 3.14159265f / 180.0f; }
    inline float rad2deg(float r) { return r * 180.0f / 3.14159265f; }
    inline float fov_to_pixels(float fov_deg, float screen_size) { return tanf(deg2rad(fov_deg) * 0.5f) * screen_size; }
}