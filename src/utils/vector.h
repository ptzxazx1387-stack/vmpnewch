#pragma once
#include <cmath>
#include <cstdint>
#include <cstring>

// Minimal math types for game cheat
struct Vec2 {
    float x, y;
    Vec2() : x(0), y(0) {}
    Vec2(float _x, float _y) : x(_x), y(_y) {}
    float length() const { return sqrtf(x * x + y * y); }
    Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
};

struct Vec3 {
    float x, y, z;
    Vec3() : x(0), y(0), z(0) {}
    Vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
    float length() const { return sqrtf(x * x + y * y + z * z); }
    float dot(const Vec3& o) const { return x * o.x + y * o.y + z * o.z; }
    Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    Vec3 operator/(float s) const { return {x / s, y / s, z / s}; }
    Vec3 normalized() const { float l = length(); return l > 0.001f ? *this / l : Vec3(); }
    float dist_to(const Vec3& o) const { return (*this - o).length(); }
};

struct Vec4 {
    float x, y, z, w;
    Vec4() : x(0), y(0), z(0), w(0) {}
    Vec4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}
    Vec4(const Vec3& v, float _w) : x(v.x), y(v.y), z(v.z), w(_w) {}
};

// Row-major 4x4 matrix (matches DirectX conventions)
struct Matrix4x4 {
    float m[4][4];

    Matrix4x4() {
        memset(this, 0, sizeof(Matrix4x4));
        m[0][0] = m[1][1] = m[2][2] = m[3][3] = 1.0f;
    }

    Vec4 mul(const Vec4& v) const {
        return {
            m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z + m[0][3] * v.w,
            m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z + m[1][3] * v.w,
            m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z + m[2][3] * v.w,
            m[3][0] * v.x + m[3][1] * v.y + m[3][2] * v.z + m[3][3] * v.w
        };
    }

    Vec3 transform_point(const Vec3& p) const {
        Vec4 r = mul({p, 1.0f});
        return {r.x, r.y, r.z};
    }
};

struct ViewMatrix : Matrix4x4 {
    bool world_to_screen(const Vec3& world, Vec2& screen, float screen_w, float screen_h) const {
        Vec4 v = mul({world.x, world.y, world.z, 1.0f});
        if (v.w < 0.01f) return false; // behind camera
        screen.x = (0.5f + v.x / v.w * 0.5f) * screen_w;
        screen.y = (0.5f - v.y / v.w * 0.5f) * screen_h; // DX Y-flip
        return (screen.x >= 0 && screen.x <= screen_w && screen.y >= 0 && screen.y <= screen_h);
    }
};