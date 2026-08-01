#include <Windows.h>
#include <TlHelp32.h>
#include <cstdio>
#include <cstring>

// ---- ntdll types ----
typedef LONG NTSTATUS;
#define STATUS_INFO_LENGTH_MISMATCH 0xC0000004

// ---- Syscall stub maker: extracts syscall ID from clean ntdll on disk ----
static DWORD GetSyscallFromDisk(const char* funcName)
{
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

    DWORD result = 0;
    auto* dos = (IMAGE_DOS_HEADER*)base;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) goto cleanup;
    auto* nt = (IMAGE_NT_HEADERS*)(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) goto cleanup;

    DWORD expRVA = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    if (!expRVA) goto cleanup;

    auto* sec = IMAGE_FIRST_SECTION(nt);

    auto rva2fo = [&](DWORD rva) -> BYTE* {
        for (WORD i = 0; i < nt->FileHeader.NumberOfSections; i++) {
            if (rva >= sec[i].VirtualAddress &&
                rva < sec[i].VirtualAddress + sec[i].Misc.VirtualSize)
                return base + sec[i].PointerToRawData + (rva - sec[i].VirtualAddress);
        }
        return NULL;
    };

    BYTE* expData = rva2fo(expRVA);
    if (!expData) goto cleanup;
    auto* exp = (IMAGE_EXPORT_DIRECTORY*)expData;

    DWORD* nameRVAs   = (DWORD*)rva2fo(exp->AddressOfNames);
    WORD*  ordinals   = (WORD*) rva2fo(exp->AddressOfNameOrdinals);
    DWORD* funcRVAs   = (DWORD*)rva2fo(exp->AddressOfFunctions);
    if (!nameRVAs || !ordinals || !funcRVAs) goto cleanup;

    for (DWORD i = 0; i < exp->NumberOfNames; i++) {
        const char* entryName = (const char*)rva2fo(nameRVAs[i]);
        if (!entryName) continue;
        if (strcmp(entryName, funcName) == 0) {
            BYTE* fnBytes = rva2fo(funcRVAs[ordinals[i]]);
            // Standard x64 syscall stub: 4C 8B D1 (mov r10, rcx) + B8 (mov eax, id)
            if (fnBytes && fnBytes[0] == 0x4C && fnBytes[1] == 0x8B && fnBytes[2] == 0xD1)
                result = *(DWORD*)(fnBytes + 4);
            break;
        }
    }

cleanup:
    UnmapViewOfFile(base);
    return result;
}

// ---- Allocate RWX stub: mov r10,rcx / mov eax,id / syscall / ret ----
static void* MakeSyscallStub(DWORD syscallId)
{
    BYTE* code = (BYTE*)VirtualAlloc(NULL, 20, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!code) return NULL;
    int i = 0;
    code[i++] = 0x4C; code[i++] = 0x8B; code[i++] = 0xD1; // mov r10, rcx
    code[i++] = 0xB8;                                        // mov eax, imm32
    *(DWORD*)(code + i) = syscallId; i += 4;
    code[i++] = 0x0F; code[i++] = 0x05;                      // syscall
    code[i++] = 0xC3;                                         // ret
    return code;
}

// ---- Find game by process name (process list NOT blocked by EAC) ----
static DWORD FindGamePid(void)
{
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    DWORD pid = 0;
    PROCESSENTRY32W pe = { sizeof(pe) };
    if (Process32FirstW(snap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, L"GTA5.exe") == 0 ||
                wcsstr(pe.szExeFile, L"GameProcess") != NULL) {
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
    printf("=== VMP Injector v5 - Direct Syscall ===\n\n");

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
        printf("Press ENTER..."); (void)getchar();
        return 1;
    }
    printf("[*] DLL: %s\n", dllPath);

    // 2. Read syscall IDs from fresh ntdll.dll on disk (NOT the hooked in-memory copy)
    printf("\n[*] Extracting syscall IDs from C:\\Windows\\System32\\ntdll.dll...\n");
    DWORD sysOpen  = GetSyscallFromDisk("NtOpenProcess");
    DWORD sysAlloc = GetSyscallFromDisk("NtAllocateVirtualMemory");
    DWORD sysWrite = GetSyscallFromDisk("NtWriteVirtualMemory");
    printf("    Syscall NtOpenProcess            = 0x%04X\n", sysOpen);
    printf("    Syscall NtAllocateVirtualMemory  = 0x%04X\n", sysAlloc);
    printf("    Syscall NtWriteVirtualMemory     = 0x%04X\n", sysWrite);

    if (!sysOpen || !sysAlloc || !sysWrite) {
        printf("[!] Failed to extract one or more syscall IDs.\n");
        printf("Press ENTER..."); (void)getchar(); return 1;
    }

    // 3. Build syscall stubs in executable memory
    typedef NTSTATUS (__stdcall *OpenProcFn)(PHANDLE, ACCESS_MASK, PVOID, PVOID);
    typedef NTSTATUS (__stdcall *AllocVMFn)(HANDLE, PVOID*, ULONG_PTR, ULONG*, ULONG, ULONG);
    typedef NTSTATUS (__stdcall *WriteVMFn)(HANDLE, PVOID, PVOID, ULONG, ULONG*);

    auto fnOpen  = (OpenProcFn)MakeSyscallStub(sysOpen);
    auto fnAlloc = (AllocVMFn) MakeSyscallStub(sysAlloc);
    auto fnWrite = (WriteVMFn)MakeSyscallStub(sysWrite);

    if (!fnOpen || !fnAlloc || !fnWrite) {
        printf("[!] Syscall stub allocation failed.\n");
        printf("Press ENTER..."); (void)getchar(); return 1;
    }

    // 4. Find game process
    printf("\n[*] Waiting for game process...\n");
    DWORD pid = 0;
    while (!pid) {
        pid = FindGamePid();
        if (!pid) Sleep(2000);
    }

    // 5. Open process via DIRECT syscall (bypasses EAC hook on NtOpenProcess)
    UINT64 clientId[2] = { (UINT64)pid, 0 };  // UniqueProcess, UniqueThread
    HANDLE hProc = NULL;
    NTSTATUS st = fnOpen(&hProc,
        PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
        PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
        NULL,          // ObjectAttributes = NULL for process handles
        clientId);     // PCLIENT_ID

    if (st < 0 || !hProc) {
        printf("[!] DEEP NtOpenProcess via raw syscall FAILED: status=0x%08lX\n", st);
        printf("    EAC kernel driver is blocking syscall-level access.\n");
        printf("Press ENTER..."); (void)getchar(); return 1;
    }
    printf("[+] Process handle: 0x%p (syscall)\n", hProc);

    // 6. Allocate & write DLL path via direct syscall
    SIZE_T pathLen = strlen(dllPath) + 1;
    PVOID  remoteBuf = NULL;
    ULONG  regionSize = (ULONG)pathLen;

    st = fnAlloc(hProc, &remoteBuf, 0, &regionSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (st < 0) {
        printf("[!] NtAllocateVirtualMemory syscall failed: 0x%08lX\n", st);
        CloseHandle(hProc);
        printf("Press ENTER..."); (void)getchar(); return 1;
    }

    st = fnWrite(hProc, remoteBuf, (PVOID)dllPath, (ULONG)pathLen, NULL);
    if (st < 0) {
        printf("[!] NtWriteVirtualMemory syscall failed: 0x%08lX\n", st);
        VirtualFreeEx(hProc, remoteBuf, 0, MEM_RELEASE);
        CloseHandle(hProc);
        printf("Press ENTER..."); (void)getchar(); return 1;
    }
    printf("[+] DLL path written to 0x%p\n", remoteBuf);

    // 7. Create remote thread via kernel32 (handle is already valid, not blocked)
    auto* pLoadLib = (LPTHREAD_START_ROUTINE)GetProcAddress(
        GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
    HANDLE hThread = CreateRemoteThread(hProc, NULL, 0, pLoadLib, remoteBuf, 0, NULL);
    if (!hThread) {
        printf("[!] CreateRemoteThread failed: error %lu\n", GetLastError());
        VirtualFreeEx(hProc, remoteBuf, 0, MEM_RELEASE);
        CloseHandle(hProc);
        printf("Press ENTER..."); (void)getchar(); return 1;
    }
    WaitForSingleObject(hThread, INFINITE);

    printf("[+] Injection complete! Press INSERT in game to open menu.\n");
    CloseHandle(hThread);
    VirtualFreeEx(hProc, remoteBuf, 0, MEM_RELEASE);
    CloseHandle(hProc);

    printf("Press ENTER to exit..."); (void)getchar();
    return 0;
}