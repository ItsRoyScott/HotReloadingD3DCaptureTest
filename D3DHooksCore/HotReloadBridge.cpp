#include "HotReloadBridge.h"

static PayloadBridge g_Payload;
static CRITICAL_SECTION g_PayloadCS;
static HANDLE g_hWatchThread = nullptr;
static bool g_Running = false;
static ID3D11Device* g_pStoredDevice = nullptr;
static ID3D11DeviceContext* g_pStoredContext = nullptr;

static void UnloadPayloadModuleInternal() {
    if (g_Payload.isLoaded) {
        std::cout << "[VERBOSE_LOG][HotReloadBridge] Unbinding existing payload module..." << std::endl;
        if (g_Payload.Shutdown) g_Payload.Shutdown();
        FreeLibrary(g_Payload.hModule);
        g_Payload = PayloadBridge();
        std::cout << "[VERBOSE_LOG][HotReloadBridge] Module unbinded cleanly." << std::endl;
    }
}

static bool LoadPayloadModuleInternal(ID3D11Device* pDevice, ID3D11DeviceContext* pContext) {
    UnloadPayloadModuleInternal();

    std::wstring sourceDll = L"SegmentationMap.dll";
    std::wstring shadowDll = L"SegmentationMap.dll.shadow";

    std::wcout << L"[VERBOSE_LOG][HotReloadBridge] Attempting to copy " << sourceDll << L" -> " << shadowDll << std::endl;

    if (!CopyFileW(sourceDll.c_str(), shadowDll.c_str(), FALSE)) {
        std::cout << "[!] [VERBOSE_LOG][HotReloadBridge] CopyFileW failed. Waiting for SegmentationMap.dll on disk..." << std::endl;
        return false;
    }

    HMODULE hMod = LoadLibraryW(shadowDll.c_str());
    if (!hMod) {
        std::cout << "[!] [VERBOSE_LOG][HotReloadBridge] LoadLibraryW failed for shadow DLL." << std::endl;
        return false;
    }

    g_Payload.hModule = hMod;
    g_Payload.Initialize = (PFN_InitializePayload)GetProcAddress(hMod, "InitializePayload");
    g_Payload.OnRenderFrame = (PFN_OnRenderFrame)GetProcAddress(hMod, "OnRenderFrame");
    g_Payload.Shutdown = (PFN_ShutdownPayload)GetProcAddress(hMod, "ShutdownPayload");

    if (g_Payload.Initialize && g_Payload.OnRenderFrame && g_Payload.Shutdown) {
        g_Payload.Initialize(pDevice, pContext);
        g_Payload.isLoaded = true;
        std::cout << "[+] [VERBOSE_LOG][HotReloadBridge] SUCCESS: SegmentationMap.dll loaded and bound." << std::endl;
    }
    else {
        std::cout << "[!] [VERBOSE_LOG][HotReloadBridge] Export functions missing in SegmentationMap.dll!" << std::endl;
        FreeLibrary(hMod);
        g_Payload = PayloadBridge();
    }
    return g_Payload.isLoaded;
}

static DWORD WINAPI DirectoryWatcherThread(LPVOID param) {
    wchar_t dirPath[MAX_PATH];
    GetCurrentDirectoryW(MAX_PATH, dirPath);

    HANDLE hDir = CreateFileW(dirPath, FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);

    if (hDir == INVALID_HANDLE_VALUE) return 0;

    BYTE buffer[1024];
    DWORD bytesReturned = 0;

    while (g_Running) {
        if (ReadDirectoryChangesW(hDir, buffer, sizeof(buffer), FALSE,
            FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME,
            &bytesReturned, nullptr, nullptr)) {

            FILE_NOTIFY_INFORMATION* pNotify = (FILE_NOTIFY_INFORMATION*)buffer;
            while (pNotify) {
                std::wstring fileName(pNotify->FileName, pNotify->FileNameLength / sizeof(wchar_t));
                if (fileName == L"SegmentationMap.dll") {
                    std::cout << "[VERBOSE_LOG][HotReloadBridge] Disk change detected on SegmentationMap.dll. Reloading..." << std::endl;
                    Sleep(200);
                    EnterCriticalSection(&g_PayloadCS);
                    LoadPayloadModuleInternal(g_pStoredDevice, g_pStoredContext);
                    LeaveCriticalSection(&g_PayloadCS);
                    break;
                }
                if (pNotify->NextEntryOffset == 0) break;
                pNotify = (FILE_NOTIFY_INFORMATION*)((BYTE*)pNotify + pNotify->NextEntryOffset);
            }
        }
    }
    CloseHandle(hDir);
    return 0;
}

void InitializeHotReloadWatcher(ID3D11Device* pDevice, ID3D11DeviceContext* pContext) {
    InitializeCriticalSection(&g_PayloadCS);
    g_pStoredDevice = pDevice;
    g_pStoredContext = pContext;
    g_Running = true;

    std::cout << "[VERBOSE_LOG][HotReloadBridge] Initializing Directory Watcher..." << std::endl;
    LoadPayloadModuleInternal(pDevice, pContext);

    g_hWatchThread = CreateThread(nullptr, 0, DirectoryWatcherThread, nullptr, 0, nullptr);
}

void ShutdownHotReloadWatcher() {
    g_Running = false;
    if (g_hWatchThread) {
        WaitForSingleObject(g_hWatchThread, 1000);
        CloseHandle(g_hWatchThread);
    }
    EnterCriticalSection(&g_PayloadCS);
    UnloadPayloadModuleInternal();
    LeaveCriticalSection(&g_PayloadCS);
    DeleteCriticalSection(&g_PayloadCS);
}

void ExecutePayloadFrame(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags, ID3D11DepthStencilView* pActiveDSV) {
    if (TryEnterCriticalSection(&g_PayloadCS)) {
        if (g_Payload.isLoaded && g_Payload.OnRenderFrame) {
            g_Payload.OnRenderFrame(pSwapChain, SyncInterval, Flags, pActiveDSV);
        }
        LeaveCriticalSection(&g_PayloadCS);
    }
}