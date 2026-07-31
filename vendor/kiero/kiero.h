#pragma once
#include <Windows.h>
#include <cstdint>
#include <type_traits>

namespace kiero {

enum class Status : uint32_t {
    UnknownError            = 0,
    NotSupported            = 1,
    ModuleNotFoundError     = 2,
    InterfaceNotFoundError  = 3,
    Success                 = 4,
};

enum class RenderType : uint32_t {
    None    = 0,
    D3D9    = 1,
    D3D10   = 2,
    D3D11   = 3,
    D3D12   = 4,
    OpenGL  = 5,
    Vulkan  = 6,
    Auto    = 7,
};

static inline Status init(RenderType renderType) {
    return Status::Success;
}

static inline void shutdown() {}

template<typename T>
static inline Status bind(uint32_t index, void** original, T replacement) {
    *original = nullptr;
    return Status::Success;
}

static inline void unbind() {}

static inline RenderType getRenderType() {
    return RenderType::D3D11;
}

} // namespace kiero