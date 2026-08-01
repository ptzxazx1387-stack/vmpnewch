#include <Windows.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>

// Manual ntdll types (avoid reliance on winternl.h across SDK versions)
typedef LONG NTSTATUS;
#define STATUS_INFO_LENGTH_MISMATCH 0xC0000004
#define STATUS_SUCCESS 0x00000000
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

typedef struct _UNICODE_STRING_T {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING_T;

typedef struct _SYSTEM_PROCESS_INFO {
    ULONG  NextEntryOffset;
    ULONG  NumberOfThreads;
    BYTE   Reserved1[48];
    struct _UNICODE_STRING_T ImageName;
    LONG   BasePriority;
    HANDLE UniqueProcessId;
    PVOID  ReservedPtr;
    SIZE_T PeakVirtualSize;
    SIZE_T VirtualSize;
    ULONG  PagefileMgg;
    PVOID  SmallestStack;
    // ... more fields follow, but we only need up to here
} SYSTEM_PROCESS_INFO;

typedef struct _CLIENT_ID {
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
} CLIENT_ID, *PCLIENT_ID;

// SystemInformationClass enum (partial)
#define SystemProcessInformation 0x00000005

// ntdll raw imports
#pragma comment(lib, "ntdll.lib")

extern "C" {
NTSTATUS NTAPI NtQuerySystemInformation(
    ULONG SystemInformationClass,
    PVOID SystemInformation,
    ULONG SystemInformationLength,
    PULONG ReturnLength OPTIONAL);

NTSTATUS NTAPI NtOpenProcess(
    PHANDLE ProcessHandle,
    ACCESS_MASK DesiredAccess,
    PVOID ObjectAttributes,
    PCLIENT_ID ClientId);

NTSTATUS NTAPI NtAllocateVirtualMemory(
    HANDLE ProcessHandle,
    PVOID *BaseAddress,
    ULONG ZeroBits,
    PULONG RegionSize,
    ULONG AllocationType,
    ULONG Protect);

NTSTATUS NTAPI NtWriteVirtualMemory(
    HANDLE ProcessHandle,
    PVOID BaseAddress,
    PVOID Buffer,
    ULONG NumberOfBytesToWrite,
    PULONG NumberOfBytesWritten OPTIONAL);

NTSTATUS NTAPI NtFreeVirtualMemory(
    HANDLE ProcessHandle,
    PVOID *BaseAddress,
    PULONG RegionSize,
    ULONG FreeType);

NTSTATUS NTAPI NtCreateThreadEx(
    PHANDLE ThreadHandle,
    ACCESS_MASK DesiredAccess,
    PVOID ObjectAttributes,
    HANDLE ProcessHandle,
    PVOID StartRoutine,
    PVOID Argument,
    ULONG CreateFlags,
    ULONG ZeroBits,
    ULONG StackSize,
    ULONG MaximumStackSize,
    PVOID AttributeList);

NTSTATUS NTAPI NtClose(HANDLE Handle);
}

// Find game PID by enumerating processes via NtQuerySystemInformation
static DWORD FindGamePid()
{
    ULONG size = 0x40000;
    BYTE* buf = (BYTE*)malloc(size);
    if (!buf) return 0;

    NTSTATUS st;
    while ((st = NtQuerySystemInformation(SystemProcessInformation, buf, size, NULL)) == STATUS_INFO_LENGTH_MISMATCH) {
        size += 0x40000;
        void* n = realloc(buf, size);
        if (!n) { free(buf); return 0; }
        buf = (BYTE*)n;
    }
    if (!NT_SUCCESS(st)) { free(buf); return 0; }

    DWORD result = 0;
    for (BYTE* p = buf; ; ) {
        auto* info = (SYSTEM_PROCESS_INFO*)p;
        if (info->ImageName.Buffer && info->ImageName.Length >= 8) {
            USHORT cnt = info->ImageName.Length / sizeof(wchar_t);
            wchar_t name[260] = { 0 };
            if (cnt > 259) cnt = 259;
            wcsncpy_s(name, info->ImageName.Buffer, cnt);

            // Match game processes
            bool match = false;
            if (_wcsicmp(name, L"fivem.exe") == 0) match = true;
            else if (_wcsicmp(name, L"GTA5.exe") == 0) match = true;
            else if (_wcsicmp(name, L"FiveM_GTAProcess.exe") == 0) match = true;
            else if (wcsstr(name, L"GameProcess")) match = true; // VMP_b3258_GameProcess.exe etc.

            if (match) {
                result = (DWORD)(DWORD_PTR)info->UniqueProcessId;
                wprintf(L"[+] Found: %s (PID=%u)\n", name, result);
                break;
            }
        }
        if (info->NextEntryOffset == 0) break;
        p += info->NextEntryOffset;
    }
    free(buf);
    return result;
}

// Inject DLL using raw Nt* syscalls
static bool InjectDLL(DWORD pid, const char* dllPath)
{
    CLIENT_ID cid = { (HANDLE)(DWORD_PTR)pid, 0 };
    HANDLE hProc = 0;
    NTSTATUS st = NtOpenProcess(&hProc,
        PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
        PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
        0, &cid);
    if (!NT_SUCCESS(st)) {
        printf("[!] NtOpenProcess failed (0x%08lX). Run as admin.\n", st);
        return false;
    }
    printf("[+] Handle 0x%p\n", hProc);

    SIZE_T len = strlen(dllPath) + 1;
    PVOID mem = 0;
    ULONG regionSize = (ULONG)len;
    st = NtAllocateVirtualMemory(hProc, &mem, 0, &regionSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!NT_SUCCESS(st)) { printf("[!] Alloc fail 0x%08lX\n", st); NtClose(hProc); return false; }

    st = NtWriteVirtualMemory(hProc, mem, (PVOID)dllPath, (ULONG)len, 0);
    if (!NT_SUCCESS(st)) {
        printf("[!] Write fail 0x%08lX\n", st);
        PVOID b = mem; ULONG s = 0;
        NtFreeVirtualMemory(hProc, &b, &s, MEM_RELEASE);
        NtClose(hProc);
        return false;
    }

    auto* fn = (PTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");

    HANDLE hThr = 0;
    st = NtCreateThreadEx(&hThr, THREAD_ALL_ACCESS, 0, hProc, (PVOID)fn, mem, 0, 0, 0, 0, 0);
    if (!NT_SUCCESS(st)) {
        printf("[!] CCRemoteThread fail 0x%08lX\n", st);
        PVOID b = mem; ULONG s = 0;
        NtFreeVirtualMemory(hProc, &b, &s, MEM_RELEASE);
        NtClose(hProc);
        return false;
    }

    WaitForSingleObject(hThr, 5000);
    NtClose(hThr);
    NtClose(hProc);
    return true;
}

int main(int argc, char** argv)
{
    printf("=== VMP Cheat Injector v3 (NtAPI) ===\n\n");

    char dllPath[MAX_PATH];
    if (argc > 1) {
        GetFullPathNameA(argv[1], MAX_PATH, dllPath, 0);
    } else {
        GetModuleFileNameA(0, dllPath, MAX_PATH);
        char* slash = strrchr(dllPath, '\\');
        if (slash) *(slash + 1) = 0;
        strcat_s(dllPath, "vmp_cheat.dll");
    }

    if (GetFileAttributesA(dllPath) == INVALID_FILE_ATTRIBUTES) {
        printf("[!] DLL not found: %s\n", dllPath);
        printf("    injector.exe [path\\to\\vmp_cheat.dll]\n");
        printf("Press any key...");
        (void)getchar();
        return 1;
    }
    printf("[*] DLL: %s\n\n", dllPath);

    printf("[*] Enumerating processes (NtQuerySystemInformation)...\n");
    DWORD pid = 0;
    while (!pid) {
        pid = FindGamePid();
        if (!pid) {
            printf("    .\n");
            Sleep(3000);
        }
    }

    printf("[*] Injecting into PID=%u...\n", pid);
    bool ok = InjectDLL(pid, dllPath);
    if (ok) {
        printf("[+] SUCCESS! Press INSERT in game for menu.\n");
    } else {
        printf("[!] FAILED.\n");
    }
    printf("Press any key to exit...");
    (void)getchar();
    return ok ? 0 : 1;
}