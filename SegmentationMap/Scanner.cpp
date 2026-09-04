#include "Scanner.h"

uintptr_t FindPattern(HMODULE hModule, const char* pattern, const char* mask) {
    MODULEINFO modInfo = {};
    GetModuleInformation(GetCurrentProcess(), hModule, &modInfo, sizeof(MODULEINFO));
    uintptr_t start = (uintptr_t)modInfo.lpBaseOfDll;
    uintptr_t size = modInfo.SizeOfImage;
    size_t patternLen = strlen(mask);

    for (uintptr_t i = 0; i < size - patternLen; ++i) {
        bool found = true;
        for (size_t j = 0; j < patternLen; ++j) {
            if (mask[j] != '?' && pattern[j] != *(char*)(start + i + j)) {
                found = false;
                break;
            }
        }
        if (found) return start + i;
    }
    return 0;
}

uintptr_t ResolveRIP(uintptr_t instructionAddr, DWORD offsetToAddress, DWORD instructionSize) {
    if (!instructionAddr) return 0;
    int32_t ripOffset = *(int32_t*)(instructionAddr + offsetToAddress);
    return instructionAddr + instructionSize + ripOffset;
}