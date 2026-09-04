#pragma once
#include <windows.h>
#include <psapi.h>
#include <cstdint>

uintptr_t FindPattern(HMODULE hModule, const char* pattern, const char* mask);
uintptr_t ResolveRIP(uintptr_t instructionAddr, DWORD offsetToAddress, DWORD instructionSize);