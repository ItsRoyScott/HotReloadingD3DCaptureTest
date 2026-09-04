#pragma once
#include "PayloadInterface.h"
#include "StagingRing.h"
#include "Serializer.h"

class PayloadController {
private:
    ID3D11Device* m_pDevice = nullptr;
    ID3D11DeviceContext* m_pContext = nullptr;
    StagingRingBuffer m_ringBuffer;
    SegmWriter m_writer;
    bool m_isRecording = false;
    bool m_ringInitialized = false;
    uint64_t m_frameIndex = 0;
    UINT m_width = 0;
    UINT m_height = 0;

public:
    void Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    void OnFrame(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags, ID3D11DepthStencilView* pActiveDSV);
    void Shutdown();
};