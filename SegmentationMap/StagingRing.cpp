#include "StagingRing.h"
#include <iostream>
#include <unordered_map>

void StagingRingBuffer::Initialize(ID3D11Device* pDevice, ID3D11Texture2D* pSrcColor, ID3D11Texture2D* pSrcStencil) {
    D3D11_TEXTURE2D_DESC cDesc;
    pSrcColor->GetDesc(&cDesc);
    m_width = cDesc.Width;
    m_height = cDesc.Height;
    m_colorFormat = cDesc.Format;

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
        m_stencilWidth = sDesc.Width;
        m_stencilHeight = sDesc.Height;

        std::cout << "[StagingRing] Color Res: " << m_width << "x" << m_height
            << " | Stencil Res: " << m_stencilWidth << "x" << m_stencilHeight << std::endl;

        sDesc.BindFlags = 0;
        sDesc.Usage = D3D11_USAGE_STAGING;
        sDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        sDesc.MiscFlags = 0;

        for (size_t i = 0; i < RING_SIZE; ++i) {
            pDevice->CreateTexture2D(&sDesc, nullptr, &m_stencilStaging[i]);
        }
    }
}

bool StagingRingBuffer::ExecuteReadback(
    ID3D11DeviceContext* pContext,
    ID3D11Texture2D* pSrcColor,
    ID3D11Texture2D* pSrcStencil,
    std::vector<uint8_t>& outBGR,
    std::vector<uint8_t>& outStencil
) {
    if (!pSrcStencil || !pSrcColor) return false;

    size_t writeSlot = m_writeIndex % RING_SIZE;

    // Issue asynchronous GPU copy to staging ring buffer
    pContext->CopySubresourceRegion(m_stencilStaging[writeSlot], 0, 0, 0, 0, pSrcStencil, 0, nullptr);
    pContext->CopySubresourceRegion(m_colorStaging[writeSlot], 0, 0, 0, 0, pSrcColor, 0, nullptr);

    // Map slot prepared two frames prior to avoid render stalls
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
        std::unordered_map<uint8_t, uint32_t> idCounts;
        const uint8_t* pStencilSrc = static_cast<const uint8_t*>(mappedStencil.pData);

        // Scale stencil sampling coordinates dynamically to color backbuffer bounds
        for (UINT y = 0; y < m_height; ++y) {
            UINT srcY = (m_stencilHeight > 0) ? (y * m_stencilHeight) / m_height : y;
            const uint32_t* pRow = reinterpret_cast<const uint32_t*>(pStencilSrc + srcY * mappedStencil.RowPitch);

            for (UINT x = 0; x < m_width; ++x) {
                UINT srcX = (m_stencilWidth > 0) ? (x * m_stencilWidth) / m_width : x;

                // Extract custom stencil bits (bits 24-31 or 0-7)
                uint8_t rawStencil = static_cast<uint8_t>((pRow[srcX] >> 24) & 0xFF);
                if (rawStencil == 0) rawStencil = static_cast<uint8_t>(pRow[srcX] & 0xFF);

                uint8_t customID = 0;
                // Exclude internal engine flags (60, 61, 128, 255)
                if (rawStencil > 0 && rawStencil != 60 && rawStencil != 61 && rawStencil != 128 && rawStencil != 255) {
                    customID = rawStencil;
                    activePixelCount++;
                    idCounts[customID]++;
                }
                outStencil[y * m_width + x] = customID;
            }
        }

        // Process color channels
        const uint8_t* pColorSrc = static_cast<const uint8_t*>(mappedColor.pData);
        for (UINT y = 0; y < m_height; ++y) {
            const uint8_t* pRow = pColorSrc + y * mappedColor.RowPitch;
            for (UINT x = 0; x < m_width; ++x) {
                size_t bgrIndex = (y * m_width + x) * 3;

                if (m_colorFormat == 24) { // DXGI_FORMAT_R10G10B10A2_UNORM
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

        // Periodic logging of frame metrics and stencil ID frequency
        if (m_writeIndex % 30 == 0) {
            std::cout << "[StagingRing] Frame readback success | Active Stencil Pixels: " << activePixelCount << std::endl;
            if (activePixelCount > 0) {
                std::cout << "[STENCIL_DEBUG] Stencil Active Pixels: " << activePixelCount << " | IDs found: ";
                for (const auto& pair : idCounts) {
                    std::cout << "ID[" << (int)pair.first << "]: " << pair.second << " px | ";
                }
                std::cout << std::endl;
            }
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
        if (m_stencilStaging[i]) {
            m_stencilStaging[i]->Release();
            m_stencilStaging[i] = nullptr;
        }
        if (m_colorStaging[i]) {
            m_colorStaging[i]->Release();
            m_colorStaging[i] = nullptr;
        }
    }
}