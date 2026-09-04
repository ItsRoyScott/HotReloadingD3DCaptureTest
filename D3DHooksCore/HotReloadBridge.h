#pragma once
#include "Framework.h"

typedef void(*PFN_InitializePayload)(ID3D11Device*, ID3D11DeviceContext*);
typedef void(*PFN_OnRenderFrame)(IDXGISwapChain*, UINT, UINT, ID3D11DepthStencilView*);
typedef void(*PFN_ShutdownPayload)();

struct PayloadBridge {
    HMODULE hModule = nullptr;
    PFN_InitializePayload Initialize = nullptr;
    PFN_OnRenderFrame OnRenderFrame = nullptr;
    PFN_ShutdownPayload Shutdown = nullptr;
    bool isLoaded = false;
};

void InitializeHotReloadWatcher(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void ShutdownHotReloadWatcher();
void ExecutePayloadFrame(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags, ID3D11DepthStencilView* pActiveDSV);