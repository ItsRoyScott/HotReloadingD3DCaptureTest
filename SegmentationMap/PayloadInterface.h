#pragma once
#include <d3d11.h>
#include <dxgi.h>

#define PAYLOAD_API __declspec(dllexport)

extern "C" {
    PAYLOAD_API void InitializePayload(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    PAYLOAD_API void OnRenderFrame(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags, ID3D11DepthStencilView* pActiveDSV);
    PAYLOAD_API void ShutdownPayload();
}