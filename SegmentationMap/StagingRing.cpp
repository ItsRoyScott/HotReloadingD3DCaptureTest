#include "StagingRing.h"
#include <iostream>

void StagingRingBuffer::Initialize(ID3D11Device* pDevice, ID3D11Texture2D* pSrcColor, ID3D11Texture2D* pSrcStencil) {
    D3D11_TEXTURE2D_DESC cDesc;
    pSrcColor->GetDesc(&cDesc);
    m_width = cDesc.Width;
    m_height = cDesc.Height;
    m_colorFormat = cDesc.Format;

    std::cout << "[StagingRing] Initialized with source format: " << m_colorFormat
        << " | Resolution: " << m_width << "x" << m_height << std::endl;

    // Inherit exact source format to prevent silent D3D11 CopySubresourceRegion failures
    cDesc.BindFlags = 0;
    cDesc.Usage = D3D11_USAGE_STAGING;
    cDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    cDesc.MiscFlags = 0;

    for (size_t i = 0; i < RING_SIZE; ++i) {
        pDevice->CreateTexture2D(&cDesc, nullptr, &m_colorStaging[i]);
    }

    if (pSrcStencil) {
        D3D11_TEXTURE2D_DESC sDesc;
        pSrcStencil->GetDesc(&sDesc);
        sDesc.BindFlags = 0;
        sDesc.Usage = D3D11_USAGE_STAGING;
        sDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        sDesc.MiscFlags = 0;

        for (size_t i = 0; i < RING_SIZE; ++i) {
            pDevice->CreateTexture2D(&sDesc, nullptr, &m_stencilStaging[i]);
        }
    }
}

bool StagingRingBuffer::ExecuteReadback(ID3D11DeviceContext* pContext, ID3D11Texture2D* pSrcColor, ID3D11Texture2D* pSrcStencil, std::vector<uint8_t>& outBGR, std::vector<uint8_t>& outStencil) {
    if (!pSrcStencil || !pSrcColor) return false;

    size_t writeSlot = m_writeIndex % RING_SIZE;

    pContext->CopySubresourceRegion(m_stencilStaging[writeSlot], 0, 0, 0, 0, pSrcStencil, 0, nullptr);
    pContext->CopySubresourceRegion(m_colorStaging[writeSlot], 0, 0, 0, 0, pSrcColor, 0, nullptr);

    size_t readSlot = (m_writeIndex + 1) % RING_SIZE;

    D3D11_MAPPED_SUBRESOURCE mappedStencil = {};
    D3D11_MAPPED_SUBRESOURCE mappedColor = {};

    HRESULT hrStencil = pContext->Map(m_stencilStaging[readSlot], 0, D3D11_MAP_READ, D3D11_MAP_FLAG_DO_NOT_WAIT, &mappedStencil);
    HRESULT hrColor = pContext->Map(m_colorStaging[readSlot], 0, D3D11_MAP_READ, D3D11_MAP_FLAG_DO_NOT_WAIT, &mappedColor);

    bool success = false;

    if (hrStencil == S_OK && hrColor == S_OK) {
        outStencil.resize(m_width * m_height);
        outBGR.resize(m_width * m_height * 3);

        uint32_t activePixelCount = 0;
        const uint8_t* pStencilSrc = static_cast<const uint8_t*>(mappedStencil.pData);
        for (UINT y = 0; y < m_height; ++y) {
            const uint32_t* pRow = reinterpret_cast<const uint32_t*>(pStencilSrc + y * mappedStencil.RowPitch);
            for (UINT x = 0; x < m_width; ++x) {
                uint8_t val = static_cast<uint8_t>((pRow[x] >> 24) & 0xFF);
                if (val == 0) val = static_cast<uint8_t>(pRow[x] & 0xFF);
                if (val > 0) activePixelCount++;
                outStencil[y * m_width + x] = val;
            }
        }

        const uint8_t* pColorSrc = static_cast<const uint8_t*>(mappedColor.pData);
        for (UINT y = 0; y < m_height; ++y) {
            const uint8_t* pRow = pColorSrc + y * mappedColor.RowPitch;
            for (UINT x = 0; x < m_width; ++x) {
                size_t bgrIndex = (y * m_width + x) * 3;

                // Handle 10-bit HDR Format (R10G10B10A2)
                if (m_colorFormat == 24) {
                    uint32_t pixel = *reinterpret_cast<const uint32_t*>(pRow + x * 4);
                    outBGR[bgrIndex + 0] = static_cast<uint8_t>(((pixel >> 20) & 0x3FF) >> 2); // B
                    outBGR[bgrIndex + 1] = static_cast<uint8_t>(((pixel >> 10) & 0x3FF) >> 2); // G
                    outBGR[bgrIndex + 2] = static_cast<uint8_t>((pixel & 0x3FF) >> 2);         // R
                }
                else {
                    size_t bgraIndex = x * 4;
                    outBGR[bgrIndex + 0] = pRow[bgraIndex + 0]; // B
                    outBGR[bgrIndex + 1] = pRow[bgraIndex + 1]; // G
                    outBGR[bgrIndex + 2] = pRow[bgraIndex + 2]; // R
                }
            }
        }

        pContext->Unmap(m_stencilStaging[readSlot], 0);
        pContext->Unmap(m_colorStaging[readSlot], 0);
        success = true;

        if (m_writeIndex % 30 == 0) {
            std::cout << "[StagingRing] Frame readback success | Active Stencil Pixels: " << activePixelCount << std::endl;
        }
    }
    else {
        if (hrStencil == S_OK) pContext->Unmap(m_stencilStaging[readSlot], 0);
        if (hrColor == S_OK) pContext->Unmap(m_colorStaging[readSlot], 0);
    }

    m_writeIndex++;
    return success;
}

void StagingRingBuffer::Release() {
    for (size_t i = 0; i < RING_SIZE; ++i) {
        if (m_stencilStaging[i]) { m_stencilStaging[i]->Release(); m_stencilStaging[i] = nullptr; }
        if (m_colorStaging[i]) { m_colorStaging[i]->Release(); m_colorStaging[i] = nullptr; }
    }
}