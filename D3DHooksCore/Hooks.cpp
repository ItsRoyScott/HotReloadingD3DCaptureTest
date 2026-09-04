#include "Hooks.h"
#include "HotReloadBridge.h"

typedef HRESULT(WINAPI* PFN_Present)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
typedef void(WINAPI* PFN_ClearDepthStencilView)(ID3D11DeviceContext* pContext, ID3D11DepthStencilView* pDepthStencilView, UINT ClearFlags, FLOAT Depth, UINT8 Stencil);

static PFN_Present oPresent = nullptr;
static PFN_ClearDepthStencilView oClearDepthStencilView = nullptr;
static ID3D11DepthStencilView* g_pActiveCustomDepthDSV = nullptr;
static bool g_Initialized = false;

static HRESULT WINAPI HookedPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
    ID3D11Device* pDevice = nullptr;
    ID3D11DeviceContext* pContext = nullptr;

    if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&pDevice))) {
        pDevice->GetImmediateContext(&pContext);

        if (!g_Initialized) {
            InitializeHotReloadWatcher(pDevice, pContext);
            g_Initialized = true;
        }

        ExecutePayloadFrame(pSwapChain, SyncInterval, Flags, g_pActiveCustomDepthDSV);

        pContext->Release();
        pDevice->Release();
    }
    return oPresent(pSwapChain, SyncInterval, Flags);
}

static void WINAPI HookedClearDepthStencilView(ID3D11DeviceContext* pContext, ID3D11DepthStencilView* pDSV, UINT ClearFlags, FLOAT Depth, UINT8 Stencil) {
    if (pDSV && (ClearFlags & D3D11_CLEAR_STENCIL)) {
        // Retain the active stencil view
        g_pActiveCustomDepthDSV = pDSV;
    }
    oClearDepthStencilView(pContext, pDSV, ClearFlags, Depth, Stencil);
}

void InitializeDirectXHooks() {
    if (MH_Initialize() != MH_OK) return;

    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, DefWindowProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"DummyDX", nullptr };
    RegisterClassEx(&wc);
    HWND hWnd = CreateWindow(L"DummyDX", L"", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, nullptr, nullptr, wc.hInstance, nullptr);

    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Width = 100;
    sd.BufferDesc.Height = 100;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;

    ID3D11Device* pDummyDevice = nullptr;
    ID3D11DeviceContext* pDummyContext = nullptr;
    IDXGISwapChain* pDummySwapChain = nullptr;

    D3D_FEATURE_LEVEL featureLevel;
    if (SUCCEEDED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &sd, &pDummySwapChain, &pDummyDevice, &featureLevel, &pDummyContext))) {

        void** pSwapChainVTable = *reinterpret_cast<void***>(pDummySwapChain);
        void** pContextVTable = *reinterpret_cast<void***>(pDummyContext);

        MH_CreateHook(pSwapChainVTable[8], &HookedPresent, reinterpret_cast<void**>(&oPresent));
        MH_CreateHook(pContextVTable[53], &HookedClearDepthStencilView, reinterpret_cast<void**>(&oClearDepthStencilView));

        MH_EnableHook(MH_ALL_HOOKS);

        pDummySwapChain->Release();
        pDummyContext->Release();
        pDummyDevice->Release();
    }

    DestroyWindow(hWnd);
    UnregisterClass(L"DummyDX", wc.hInstance);
}

void ShutdownDirectXHooks() {
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
}