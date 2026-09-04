#include "Framework.h"
#include "Hooks.h"

DWORD WINAPI CoreThread(LPVOID lpParam) {
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    std::cout << "==========================================" << std::endl;
    std::cout << " HotReloadingD3DCapture Core Hook Injected" << std::endl;
    std::cout << " Target: The Baby In Yellow (-d3d11)" << std::endl;
    std::cout << "==========================================" << std::endl;

    InitializeDirectXHooks();
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, CoreThread, nullptr, 0, nullptr);
    } else if (ul_reason_for_call == DLL_PROCESS_DETACH) {
        ShutdownDirectXHooks();
    }
    return TRUE;
}