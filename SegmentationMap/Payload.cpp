#include "Payload.h"
#include <iostream>

static PayloadController g_Controller;

void PayloadController::Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext) {
    m_pDevice = pDevice;
    m_pContext = pContext;
    std::cout << "[SegmentationMap.dll] Initialized standalone capture payload." << std::endl;
}

void PayloadController::OnFrame(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags, ID3D11DepthStencilView* pActiveDSV) {
    if (GetAsyncKeyState(VK_F8) & 1) {
        m_isRecording = !m_isRecording;
        if (m_isRecording) {
            DXGI_SWAP_CHAIN_DESC desc;
            pSwapChain->GetDesc(&desc);
            m_width = desc.BufferDesc.Width;
            m_height = desc.BufferDesc.Height;

            m_writer.Open("capture_output.segm", static_cast<uint16_t>(m_width), static_cast<uint16_t>(m_height));
            std::cout << "[+] RECORDING STARTED: " << m_width << "x" << m_height << " -> capture_output.segm" << std::endl;
            //std::cout << "hello world - testing hot reloading" << std::endl;
        }
        else {
            m_writer.Close();
            std::cout << "[-] RECORDING STOPPED. File saved." << std::endl;
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
    if (m_isRecording) m_writer.Close();
    m_ringBuffer.Release();
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