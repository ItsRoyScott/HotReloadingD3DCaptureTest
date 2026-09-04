#pragma once
#include <windows.h>
#include <cstdint>
#include <vector>
#include <fstream>
#include <string>
#include <turbojpeg.h>

#pragma pack(push, 1)
struct SegmHeader {
    char magic[4] = { 'S', 'E', 'G', 'M' };
    uint32_t version = 0x00030000;
    uint16_t width;
    uint16_t height;
    uint16_t frameRate = 60;
    uint8_t compressionType = 2; // TurboJPEG Color + RLE Stencil
    uint8_t reserved[17] = { 0 };
    uint64_t totalFrames = 0;
    uint64_t dictionaryOffset = 0;
    uint64_t payloadOffset = 0x40;
};

struct FramePacketHeader {
    uint64_t frameIndex;
    uint64_t timestampMicroseconds;
    uint32_t colorCompSize;     // TurboJPEG compressed color size
    uint32_t stencilCompSize;   // RLE compressed stencil size
    uint32_t stencilUncompSize; // Uncompressed stencil size
};
#pragma pack(pop)

class SegmWriter {
private:
    std::ofstream m_file;
    SegmHeader m_header;
    uint64_t m_frameCount = 0;
    tjhandle m_jpegCompressor = nullptr;

public:
    bool Open(const std::string& filepath, uint16_t width, uint16_t height);
    void WriteFrame(uint64_t frameIndex, uint64_t timestamp, const std::vector<uint8_t>& colorBGR, const std::vector<uint8_t>& rawStencil);
    void Close();
};