#include "Payload.h"
#include "Scanner.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <windows.h>

static PayloadController g_Controller;
static std::ofstream g_LogFile;

// Dual logging helper: writes to std::cout and recording_output.log
static void LogLine(const std::string& text) {
    std::cout << text << std::endl;
    if (g_LogFile.is_open()) {
        g_LogFile << text << std::endl;
        g_LogFile.flush();
    }
}

// UE4 Reflection & Object Array Structures
typedef void(*PFN_ProcessEvent)(void* pObject, void* pFunction, void* pParms);
static PFN_ProcessEvent g_ProcessEvent = nullptr;

struct FUObjectItem {
    void* Object;
    int32_t Flags;
    int32_t ClusterRootIndex;
    int32_t SerialNumber;
};

struct GObjectsArray {
    FUObjectItem** Objects;
    FUObjectItem* PreAllocatedObjects;
    int32_t MaxElements;
    int32_t NumElements;
    int32_t MaxChunks;
    int32_t NumChunks;
};

static GObjectsArray* g_pGObjects = nullptr;

// Diagnostic 1: Log active stencil ID distribution per frame
void LogStencilHistogram(const std::vector<uint8_t>& rawStencil, UINT width, UINT height) {
    std::unordered_map<uint8_t, size_t> histogram;
    for (uint8_t val : rawStencil) {
        if (val != 0) histogram[val]++;
    }

    std::stringstream ss;
    if (!histogram.empty()) {
        ss << "[STENCIL_HISTOGRAM] Frame Active IDs: ";
        for (auto& [id, count] : histogram) {
            ss << "ID[" << (int)id << "]: " << count << " px | ";
        }
    }
    else {
        ss << "[STENCIL_HISTOGRAM] 0 active stencil pixels on current frame.";
    }
    LogLine(ss.str());
}

// Helper function to safely attempt memory reads without crashing on invalid addresses
static bool SafeReadComponentCustomDepth(uintptr_t objAddr, bool& outRenderCustomDepth, int32_t& outStencilVal) {
    __try {
        outRenderCustomDepth = *reinterpret_cast<bool*>(objAddr + 0x228);
        outStencilVal = *reinterpret_cast<int32_t*>(objAddr + 0x22c);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// Diagnostic 2: Dump custom depth settings safely across valid primitive components
void DumpEnginePrimitiveComponentStates() {
    if (!g_pGObjects || !g_pGObjects->Objects) {
        LogLine("[DIAG_DUMP] GUObjectArray not resolved or empty.");
        return;
    }

    LogLine("\n========== [GUOBJECTARRAY PRIMITIVE COMPONENT DUMP] ==========");
    int32_t totalCount = g_pGObjects->NumElements;
    int dumpedCount = 0;

    for (int32_t i = 0; i < totalCount; ++i) {
        FUObjectItem* pChunk = g_pGObjects->Objects[i / 65536];
        if (!pChunk) continue;

        FUObjectItem& item = pChunk[i % 65536];
        void* pObj = item.Object;

        if (!pObj || item.Flags == 0) continue;

        uintptr_t objAddr = reinterpret_cast<uintptr_t>(pObj);

        bool bRenderCustomDepth = false;
        int32_t customDepthStencilValue = 0;

        if (SafeReadComponentCustomDepth(objAddr, bRenderCustomDepth, customDepthStencilValue)) {
            if (bRenderCustomDepth && (customDepthStencilValue >= 0 && customDepthStencilValue <= 255)) {
                std::stringstream ss;
                ss << "  -> Found CustomDepth Active Obj [" << i << "] at 0x" << std::hex << objAddr
                    << " | bRenderCustomDepth: " << (bRenderCustomDepth ? "TRUE" : "FALSE")
                    << " | StencilVal: " << std::dec << customDepthStencilValue;
                LogLine(ss.str());
                dumpedCount++;
            }
        }
    }

    std::stringstream ssEnd;
    ssEnd << "========== [END DUMP: " << dumpedCount << " Active CustomDepth Objects] ==========\n";
    LogLine(ssEnd.str());
}

void PayloadController::Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext) {
    m_pDevice = pDevice;
    m_pContext = pContext;

    // Open log file in append mode
    g_LogFile.open("recording_output.log", std::ios::out | std::ios::app);
    LogLine("[SegmentationMap.dll] Initialized diagnostic capture payload.");

    HMODULE hGame = GetModuleHandle(nullptr);

    // Resolve ProcessEvent pattern
    uintptr_t processEventAddr = FindPattern(hGame, "\x40\x55\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x81\xEC\x00\x00\x00\x00", "xxxxxxxxxxxxxxx????");
    if (processEventAddr) {
        g_ProcessEvent = reinterpret_cast<PFN_ProcessEvent>(processEventAddr);
        std::stringstream ss;
        ss << "[Reflection] Resolved ProcessEvent: 0x" << std::hex << processEventAddr;
        LogLine(ss.str());
    }

    // Resolve GUObjectArray pattern
    uintptr_t gObjectsAddr = FindPattern(hGame, "\x48\x8B\x05\x00\x00\x00\x00\x48\x8B\x0C\xC8", "xxx????xxxx");
    if (gObjectsAddr) {
        uintptr_t resolvedGObjects = ResolveRIP(gObjectsAddr, 3, 7);
        g_pGObjects = reinterpret_cast<GObjectsArray*>(resolvedGObjects);
        std::stringstream ss;
        ss << "[Reflection] Resolved GUObjectArray: 0x" << std::hex << resolvedGObjects;
        LogLine(ss.str());
    }
}

void PayloadController::OnFrame(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags, ID3D11DepthStencilView* pActiveDSV) {
    // F8 keypress triggers logging dump and recording toggle
    if (GetAsyncKeyState(VK_F8) & 1) {
        m_isRecording = !m_isRecording;
        if (m_isRecording) {
            DXGI_SWAP_CHAIN_DESC desc;
            pSwapChain->GetDesc(&desc);
            m_width = desc.BufferDesc.Width;
            m_height = desc.BufferDesc.Height;

            DumpEnginePrimitiveComponentStates();

            m_writer.Open("capture_output.segm", static_cast<uint16_t>(m_width), static_cast<uint16_t>(m_height));
            LogLine("[+] RECORDING STARTED with Enhanced Diagnostics.");
            LogLine("hello world! testing hot reloading");
        }
        else {
            m_writer.Close();
            m_ringInitialized = false;
            LogLine("[-] RECORDING STOPPED.");
        }
    }

    if (m_isRecording && pActiveDSV) {
        ID3D11Texture2D* pBackBuffer = nullptr;
        if (SUCCEEDED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer))) {
            ID3D11Resource* pDSVRes = nullptr;
            pActiveDSV->GetResource(&pDSVRes);
            if (pDSVRes) {
                ID3D11Texture2D* pStencilTex = nullptr;
                if (SUCCEEDED(pDSVRes->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&pStencilTex))) {

                    if (!m_ringInitialized) {
                        m_ringBuffer.Initialize(m_pDevice, pBackBuffer, pStencilTex);
                        m_ringInitialized = true;
                    }

                    std::vector<uint8_t> colorBGR;
                    std::vector<uint8_t> rawStencil;

                    if (m_ringBuffer.ExecuteReadback(m_pContext, pBackBuffer, pStencilTex, colorBGR, rawStencil)) {
                        uint64_t timestamp = GetTickCount64() * 1000;

                        LogStencilHistogram(rawStencil, m_width, m_height);
                        m_writer.WriteFrame(m_frameIndex++, timestamp, colorBGR, rawStencil);
                    }
                    pStencilTex->Release();
                }
                pDSVRes->Release();
            }
            pBackBuffer->Release();
        }
    }
}

void PayloadController::Shutdown() {
    if (m_isRecording) {
        m_writer.Close();
    }
    m_ringBuffer.Release();
    m_ringInitialized = false;

    if (g_LogFile.is_open()) {
        LogLine("[SegmentationMap.dll] Payload shutdown.");
        g_LogFile.close();
    }
}

void InitializePayload(ID3D11Device* pDevice, ID3D11DeviceContext* pContext) {
    g_Controller.Initialize(pDevice, pContext);
}

void OnRenderFrame(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags, ID3D11DepthStencilView* pActiveDSV) {
    g_Controller.OnFrame(pSwapChain, SyncInterval, Flags, pActiveDSV);
}

void ShutdownPayload() {
    g_Controller.Shutdown();
}