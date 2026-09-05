#pragma once
#include <d3d11.h>
#include <vector>
#include <cstdint>

class StagingRingBuffer {
private:
    static constexpr size_t RING_SIZE = 3;
    ID3D11Texture2D* m_stencilStaging[RING_SIZE] = { nullptr };
    ID3D11Texture2D* m_colorStaging[RING_SIZE] = { nullptr };
    size_t m_writeIndex = 0;
    UINT m_width = 0;
    UINT m_height = 0;
    UINT m_stencilWidth = 0;
    UINT m_stencilHeight = 0;
    DXGI_FORMAT m_colorFormat = DXGI_FORMAT_UNKNOWN;

public:
    void Initialize(ID3D11Device* pDevice, ID3D11Texture2D* pSrcColor, ID3D11Texture2D* pSrcStencil);
    bool ExecuteReadback(
        ID3D11DeviceContext* pContext,
        ID3D11Texture2D* pSrcColor,
        ID3D11Texture2D* pSrcStencil,
        std::vector<uint8_t>& outBGR,
        std::vector<uint8_t>& outStencil
    );
    void Release();
};