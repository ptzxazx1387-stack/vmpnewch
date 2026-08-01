#include <Windows.h>
#include <TlHelp32.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>

// Find ANY process that has gta-core-five.dll loaded.
// Avoids hardcoded process names like VMP_b3258_GameProcess.exe
static DWORD FindGameProcess() {
	HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snap == INVALID_HANDLE_VALUE) return 0;

	DWORD result = 0;
	PROCESSENTRY32W pe = { sizeof(pe) };
	if (Process32FirstW(snap, &pe)) {
		do {
			if (pe.th32ProcessID == 0) continue;
			// Skip small / system processes to save time
			if (pe.cntThreads < 5) continue;

			// Check if this process has gta-core-five.dll loaded
			HANDLE modSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pe.th32ProcessID);
			if (modSnap == INVALID_HANDLE_VALUE) continue;

			MODULEENTRY32W me = { sizeof(me) };
			if (Module32FirstW(modSnap, &me)) {
				do {
					if (_wcsicmp(me.szModule, L"gta-core-five.dll") == 0) {
						result = pe.th32ProcessID;
						break;
					}
				} while (Module32NextW(modSnap, &me));
			}
			CloseHandle(modSnap);
			if (result) break;
		} while (Process32NextW(snap, &pe));
	}
	CloseHandle(snap);
	return result;
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

	// Scan EVERY process for one that has gta-core-five.dll loaded.
	// VMP uses custom process names like "VMP_b3258_GameProcess.exe"
	// so hardcoding names doesn't work.
	printf("[*] Searching for game process with gta-core-five.dll...\n");
	printf("    (This may take a few seconds)\n");

	DWORD pid = 0;
	while (!pid) {
		pid = FindGameProcess();
		if (!pid) {
			printf("    .\n");
			Sleep(2000);
		}
	}
	printf("[+] Game PID=%lu  (gta-core-five.dll found)\n", pid);

	bool ok = InjectDLL(pid, dllPath);
	printf(ok ? "[+] SUCCESS! Press INSERT in-game to open menu.\n" : "[!] Injection FAILED.\n");
	printf("Press any key to exit..."); (void)getchar();
	return ok ? 0 : 1;
}