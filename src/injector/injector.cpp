#include <Windows.h>
#include <TlHelp32.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>

static DWORD FindProcessId(const wchar_t* name) {
	DWORD pid = 0;
	HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snap == INVALID_HANDLE_VALUE) return 0;
	PROCESSENTRY32W pe = { sizeof(pe) };
	if (Process32FirstW(snap, &pe)) {
		do {
			if (_wcsicmp(pe.szExeFile, name) == 0) { pid = pe.th32ProcessID; break; }
		} while (Process32NextW(snap, &pe));
	}
	CloseHandle(snap);
	return pid;
}

static bool ModuleExists(DWORD pid, const wchar_t* modName) {
	HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
	if (snap == INVALID_HANDLE_VALUE) return false;
	MODULEENTRY32W me = { sizeof(me) };
	bool found = false;
	if (Module32FirstW(snap, &me)) {
		do {
			if (_wcsicmp(me.szModule, modName) == 0) { found = true; break; }
		} while (Module32NextW(snap, &me));
	}
	CloseHandle(snap);
	return found;
}

static bool InjectDLL(DWORD pid, const char* dllPath) {
	HANDLE hProc = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
		PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, pid);
	if (!hProc) {
		printf("[!] OpenProcess failed (PID=%lu, err=%lu)\n", pid, GetLastError());
		return false;
	}
	printf("[+] Opened PID=%lu\n", pid);

	size_t len = strlen(dllPath) + 1;
	LPVOID mem = VirtualAllocEx(hProc, nullptr, len, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!mem) { printf("[!] VirtualAllocEx failed\n"); CloseHandle(hProc); return false; }

	WriteProcessMemory(hProc, mem, dllPath, len, nullptr);

	auto loadLib = (LPTHREAD_START_ROUTINE)GetProcAddress(
		GetModuleHandleA("kernel32.dll"), "LoadLibraryA");

	HANDLE hThread = CreateRemoteThread(hProc, nullptr, 0, loadLib, mem, 0, nullptr);
	if (!hThread) {
		printf("[!] CreateRemoteThread failed (err=%lu)\n", GetLastError());
		VirtualFreeEx(hProc, mem, 0, MEM_RELEASE);
		CloseHandle(hProc);
		return false;
	}

	printf("[+] Injecting...\n");
	WaitForSingleObject(hThread, 5000);
	CloseHandle(hThread);
	VirtualFreeEx(hProc, mem, 0, MEM_RELEASE);
	CloseHandle(hProc);
	return true;
}

int main(int argc, char** argv) {
	printf("=== VMP Cheat Injector ===\n\n");

	char dllPath[MAX_PATH];
	if (argc > 1) {
		GetFullPathNameA(argv[1], MAX_PATH, dllPath, nullptr);
	} else {
		GetModuleFileNameA(nullptr, dllPath, MAX_PATH);
		char* slash = strrchr(dllPath, '\\');
		if (slash) *(slash + 1) = '\0';
		strcat_s(dllPath, "vmp_cheat.dll");
	}

	if (GetFileAttributesA(dllPath) == INVALID_FILE_ATTRIBUTES) {
		printf("[!] DLL not found: %s\n", dllPath);
		printf("    Usage: injector.exe [path\\to\\vmp_cheat.dll]\n");
		printf("Press any key to exit...");
		(void)getchar();
		return 1;
	}
	printf("[*] DLL: %s\n\n", dllPath);

	// VMP launcher (VMP.exe) does NOT load gta-core-five.dll.
	// We must find the GTA child process which actually holds the DLL.
	printf("[*] Scanning for game process with gta-core-five.dll...\n");
	const wchar_t* names[] = { L"GTA5.exe", L"FiveM_GTAProcess.exe", L"FiveM.exe", L"VMP.exe" };
	DWORD pid = 0;
	while (!pid) {
		for (auto& n : names) {
			DWORD candidate = FindProcessId(n);
			if (candidate && ModuleExists(candidate, L"gta-core-five.dll")) {
				pid = candidate;
				break;
			}
		}
		if (!pid) Sleep(1000);
	}
	printf("[+] Game PID=%lu  (gta-core-five.dll already present)\n", pid);

	bool ok = InjectDLL(pid, dllPath);
	printf(ok ? "[+] SUCCESS! Press INSERT in-game to open menu.\n" : "[!] Injection FAILED.\n");
	printf("Press any key to exit..."); (void)getchar();
	return ok ? 0 : 1;
}