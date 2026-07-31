#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace scanner {

struct Pattern {
    std::vector<uint8_t> bytes;
    std::string mask;
};

Pattern Parse(const char* ida_pattern);
uintptr_t FindPattern(uintptr_t start, size_t len, const char* pat);
uintptr_t FindModulePattern(const wchar_t* mod, const char* pat);
uintptr_t GetModuleBase(const wchar_t* name);
size_t GetModuleSize(uintptr_t base);
uintptr_t ResolvePointer(uintptr_t addr, uintptr_t off = 3, uintptr_t ilen = 7);
uintptr_t GetRelativeAddr(uintptr_t addr, uintptr_t off = 3);

}