#include "scanner.h"
#include <Windows.h>
#include <Psapi.h>
#include <algorithm>

namespace scanner {

Pattern Parse(const char* ida_pattern) {
    Pattern p;
    const char* s = ida_pattern;
    while (*s && (s[0] != '\0')) {
        if (*s == ' ' || *s == '\t') { s++; continue; }
        if (s[0] == '?' && s[1] == '?') { // double ?
            p.bytes.push_back(0x00);
            p.mask.push_back('?');
            s += 2;
            continue;
        }
        // single hex byte
        char hex[3] = { *s, *(s+1), '\0' };
        p.bytes.push_back((uint8_t)strtoul(hex, nullptr, 16));
        p.mask.push_back('x');
        s += 2;
    }
    return p;
}

uintptr_t FindPattern(uintptr_t start, size_t len, const char* ida_pattern) {
    auto p = Parse(ida_pattern);
    if (p.bytes.empty() || p.bytes.size() > len) return 0;

    auto* scan = (const uint8_t*)start;
    for (size_t i = 0; i <= len - p.bytes.size(); i++) {
        bool found = true;
        for (size_t j = 0; j < p.bytes.size(); j++) {
            if (p.mask[j] != '?') {
                found = (scan[i + j] == p.bytes[j]);
            }
            if (!found) break;
        }
        if (found) return start + i;
    }
    return 0;
}

uintptr_t FindModulePattern(const wchar_t* module_name, const char* pattern) {
    uintptr_t base = GetModuleBase(module_name);
    if (!base) return 0;
    size_t size = GetModuleSize(base);
    return FindPattern(base, size, pattern);
}

uintptr_t GetModuleBase(const wchar_t* name) {
    HMODULE mod = GetModuleHandleW(name);
    if (!mod) return 0;
    return (uintptr_t)mod;
}

size_t GetModuleSize(uintptr_t base) {
    MODULEINFO info;
    if (GetModuleInformation(GetCurrentProcess(), (HMODULE)base, &info, sizeof(info)))
        return info.SizeOfImage;
    return 0;
}

uintptr_t ResolvePointer(uintptr_t addr, uintptr_t offset, uintptr_t insn_len) {
    int32_t rel = *(int32_t*)(addr + offset);
    return addr + insn_len + rel;
}

uintptr_t GetRelativeAddr(uintptr_t addr, uintptr_t offset) {
    return ResolvePointer(addr, offset, 4 + offset - 3);
}

} // namespace scanner