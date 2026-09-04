#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <string>

static DWORD GetProcessIdByName(const std::wstring& processName) {
    DWORD pid = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W entry = { sizeof(PROCESSENTRY32W) };
        if (Process32FirstW(snapshot, &entry)) {
            do {
                if (processName == entry.szExeFile) {
                    pid = entry.th32ProcessID;
                    break;
                }
            } while (Process32NextW(snapshot, &entry));
        }
        CloseHandle(snapshot);
    }
    return pid;
}

int wmain(int argc, wchar_t* argv[]) {
    std::wstring targetProcess = (argc > 1) ? argv[1] : L"Game-Win64-Shipping.exe";
    std::wstring dllName = (argc > 2) ? argv[2] : L"D3DHooksCore.dll";

    std::wcout << L"[*] Searching for target process: " << targetProcess << std::endl;
    DWORD pid = GetProcessIdByName(targetProcess);
    if (!pid) {
        std::wcerr << L"[!] Process not found. Ensure " << targetProcess << L" is running." << std::endl;
        return 1;
    }

    wchar_t fullPath[MAX_PATH];
    if (GetFullPathNameW(dllName.c_str(), MAX_PATH, fullPath, nullptr) == 0) {
        std::wcerr << L"[!] Failed to resolve full path for " << dllName << std::endl;
        return 1;
    }

    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProcess) {
        std::wcerr << L"[!] Could not open process PID: " << pid << std::endl;
        return 1;
    }

    size_t pathSize = (wcslen(fullPath) + 1) * sizeof(wchar_t);
    LPVOID remoteMem = VirtualAllocEx(hProcess, nullptr, pathSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remoteMem) {
        std::wcerr << L"[!] Memory allocation failed in target process." << std::endl;
        CloseHandle(hProcess);
        return 1;
    }

    if (!WriteProcessMemory(hProcess, remoteMem, fullPath, pathSize, nullptr)) {
        std::wcerr << L"[!] Failed to write DLL path into target process." << std::endl;
        VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return 1;
    }

    LPVOID loadLibAddr = (LPVOID)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW");
    HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0, (LPTHREAD_START_ROUTINE)loadLibAddr, remoteMem, 0, nullptr);

    if (hThread) {
        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
        std::wcout << L"[+] Successfully injected " << fullPath << L" into PID " << pid << std::endl;
    } else {
        std::wcerr << L"[!] Remote thread creation failed." << std::endl;
    }

    VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
    CloseHandle(hProcess);
    return 0;
}