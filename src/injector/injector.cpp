#include <Windows.h>
#include <TlHelp32.h>
#include <cstdio>
#include <cstring>

typedef LONG NTSTATUS;

// ---- Read syscall ID from fresh ntdll.dll on disk ----
static DWORD GetSyscallFromDisk(const char* funcName)
{
    DWORD result = 0;
    HANDLE hFile = CreateFileW(L"C:\\Windows\\System32\\ntdll.dll",
        GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return 0;

    DWORD hi = 0, lo = GetFileSize(hFile, &hi);
    HANDLE hMap = CreateFileMappingW(hFile, NULL, PAGE_READONLY, hi, lo, NULL);
    CloseHandle(hFile);
    if (!hMap) return 0;

    BYTE* base = (BYTE*)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(hMap);
    if (!base) return 0;

    // Validate PE
    auto* dos = (IMAGE_DOS_HEADER*)base;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) { UnmapViewOfFile(base); return 0; }
    auto* nthdr = (IMAGE_NT_HEADERS*)(base + dos->e_lfanew);
    if (nthdr->Signature != IMAGE_NT_SIGNATURE) { UnmapViewOfFile(base); return 0; }

    DWORD exportRVA = nthdr->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    if (!exportRVA) { UnmapViewOfFile(base); return 0; }

    IMAGE_SECTION_HEADER* sections = IMAGE_FIRST_SECTION(nthdr);
    WORD numSections = nthdr->FileHeader.NumberOfSections;

    // RVA -> file offset helper
    auto rva2fo = [&](DWORD rva) -> BYTE* {
        for (WORD i = 0; i < numSections; i++) {
            if (rva >= sections[i].VirtualAddress &&
                rva < sections[i].VirtualAddress + sections[i].Misc.VirtualSize) {
                return base + sections[i].PointerToRawData + (rva - sections[i].VirtualAddress);
            }
        }
        return NULL;
    };

    BYTE* exportBase = rva2fo(exportRVA);
    if (!exportBase) { UnmapViewOfFile(base); return 0; }

    IMAGE_EXPORT_DIRECTORY* exp = (IMAGE_EXPORT_DIRECTORY*)exportBase;
    DWORD* nameRVAs   = (DWORD*)rva2fo(exp->AddressOfNames);
    WORD*  ordinals   = (WORD*) rva2fo(exp->AddressOfNameOrdinals);
    DWORD* funcRVAs   = (DWORD*)rva2fo(exp->AddressOfFunctions);
    if (!nameRVAs || !ordinals || !funcRVAs) { UnmapViewOfFile(base); return 0; }

    for (DWORD i = 0; i < exp->NumberOfNames; i++) {
        const char* entryName = (const char*)rva2fo(nameRVAs[i]);
        if (!entryName) continue;
        if (strcmp(entryName, funcName) == 0) {
            BYTE* fnBytes = rva2fo(funcRVAs[ordinals[i]]);
            // Standard x64 Nt* stub: 4C 8B D1 = mov r10, rcx / B8 = mov eax, imm
            if (fnBytes && fnBytes[0] == 0x4C && fnBytes[1] == 0x8B && fnBytes[2] == 0xD1) {
                result = *(DWORD*)(fnBytes + 4);
            }
            break;
        }
    }
    UnmapViewOfFile(base);
    return result;
}

// ---- Allocate RWX stub: mov r10,rcx / mov eax,id / syscall / ret ----
static void* MakeSyscallStub(DWORD id)
{
    BYTE* code = (BYTE*)VirtualAlloc(NULL, 20, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!code) return NULL;
    int i = 0;
    code[i++] = 0x4C; code[i++] = 0x8B; code[i++] = 0xD1; // mov r10, rcx
    code[i++] = 0xB8;                                        // mov eax, imm32
    *(DWORD*)(code + i) = id; i += 4;
    code[i++] = 0x0F; code[i++] = 0x05;                      // syscall
    code[i++] = 0xC3;                                         // ret
    return code;
}

// ---- Find game PID via process snapshot (usually not blocked by EAC) ----
static DWORD FindGamePid(void)
{
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    DWORD pid = 0;
    PROCESSENTRY32W pe = { sizeof(pe) };
    if (Process32FirstW(snap, &pe)) {
        do {
            if (wcsstr(pe.szExeFile, L"GameProcess")) {
                pid = pe.th32ProcessID;
                wprintf(L"[+] Found: %s (PID=%u)\n", pe.szExeFile, pid);
                break;
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return pid;
}

int main(int argc, char** argv)
{
    printf("=== VMP Injector v5 (Direct Syscall) ===\n\n");

    // 1. DLL path
    char dllPath[MAX_PATH];
    if (argc > 1) {
        GetFullPathNameA(argv[1], MAX_PATH, dllPath, NULL);
    } else {
        GetModuleFileNameA(NULL, dllPath, MAX_PATH);
        char* slash = strrchr(dllPath, '\\');
        if (slash) *(slash + 1) = '\0';
        strcat_s(dllPath, "vmp_cheat.dll");
    }
    if (GetFileAttributesA(dllPath) == INVALID_FILE_ATTRIBUTES) {
        printf("[!] DLL not found: %s\n", dllPath);
        printf("Press ENTER..."); (void)getchar(); return 1;
    }
    printf("[*] DLL: %s\n", dllPath);

    // 2. Extract syscall IDs from clean ntdll on disk
    printf("\n[*] Extracting syscall IDs from C:\\Windows\\System32\\ntdll.dll...\n");
    DWORD sysOpen  = GetSyscallFromDisk("NtOpenProcess");
    DWORD sysAlloc = GetSyscallFromDisk("NtAllocateVirtualMemory");
    DWORD sysWrite = GetSyscallFromDisk("NtWriteVirtualMemory");
    printf("    NtOpenProcess            = 0x%04X\n", sysOpen);
    printf("    NtAllocateVirtualMemory  = 0x%04X\n", sysAlloc);
    printf("    NtWriteVirtualMemory     = 0x%04X\n", sysWrite);
    if (!sysOpen || !sysAlloc || !sysWrite) {
        printf("[!] Failed to extract syscall IDs.\n");
        printf("Press ENTER..."); (void)getchar(); return 1;
    }

    // 3. Build syscall stubs
    typedef NTSTATUS (__stdcall *OpenProcFn)(PHANDLE, ACCESS_MASK, PVOID, PVOID);
    typedef NTSTATUS (__stdcall *AllocFn)(HANDLE, PVOID*, ULONG_PTR, ULONG*, ULONG, ULONG);
    typedef NTSTATUS (__stdcall *WriteFn)(HANDLE, PVOID, PVOID, ULONG, ULONG*);

    OpenProcFn fnOpen = (OpenProcFn)MakeSyscallStub(sysOpen);
    AllocFn    fnAlloc = (AllocFn)MakeSyscallStub(sysAlloc);
    WriteFn    fnWrite = (WriteFn)MakeSyscallStub(sysWrite);
    if (!fnOpen || !fnAlloc || !fnWrite) {
        printf("[!] Stub alloc failed.\n");
        printf("Press ENTER..."); (void)getchar(); return 1;
    }

    // 4. Find game
    printf("\n[*] Waiting for game...\n");
    DWORD pid = 0;
    while (!pid) {
        pid = FindGamePid();
        if (!pid) Sleep(2000);
    }

    // 5. Open process via DIRECT syscall (bypasses EAC hook)
    UINT64 cid[2] = { pid, 0 }; // CLIENT_ID struct
    HANDLE hProc = NULL;
    NTSTATUS st = fnOpen(&hProc,
        PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
        PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
        NULL, cid);

    if (st < 0 || !hProc) {
        printf("[!] NtOpenProcess via raw syscall: 0x%08lX\n", st);
        printf("    EAC kernel driver is blocking.\n");
        printf("Press ENTER..."); (void)getchar(); return 1;
    }
    printf("[+] Handle: 0x%p (raw syscall)\n", hProc);

    // 6. Allocate + write DLL path via syscall
    SIZE_T len = strlen(dllPath) + 1;
    PVOID remote = NULL;
    ULONG region = (ULONG)len;

    st = fnAlloc(hProc, &remote, 0, &region, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (st < 0) {
        printf("[!] AllocVirtualMemory: 0x%08lX\n", st);
        CloseHandle(hProc);
        printf("Press ENTER..."); (void)getchar(); return 1;
    }

    st = fnWrite(hProc, remote, (PVOID)dllPath, (ULONG)len, NULL);
    if (st < 0) {
        printf("[!] WriteVirtualMemory: 0x%08lX\n", st);
        VirtualFreeEx(hProc, remote, 0, MEM_RELEASE);
        CloseHandle(hProc);
        printf("Press ENTER..."); (void)getchar(); return 1;
    }

    // 7. Create remote thread via kernel32
    auto* pLoadLib = (LPTHREAD_START_ROUTINE)GetProcAddress(
        GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
    HANDLE hThread = CreateRemoteThread(hProc, NULL, 0, pLoadLib, remote, 0, NULL);
    if (!hThread) {
        printf("[!] CreateRemoteThread failed: %lu\n", GetLastError());
        VirtualFreeEx(hProc, remote, 0, MEM_RELEASE);
        CloseHandle(hProc);
        printf("Press ENTER..."); (void)getchar(); return 1;
    }
    WaitForSingleObject(hThread, INFINITE);

    printf("[+] SUCCESS! Press INSERT in-game.\n");
    CloseHandle(hThread);
    VirtualFreeEx(hProc, remote, 0, MEM_RELEASE);
    CloseHandle(hProc);

    printf("Press ENTER to exit..."); (void)getchar();
    return 0;
}