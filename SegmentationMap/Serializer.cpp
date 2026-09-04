#include "Serializer.h"

static std::vector<uint8_t> CompressRLE(const uint8_t* input, size_t size) {
    std::vector<uint8_t> compressed;
    compressed.reserve(size / 10);
    size_t i = 0;
    while (i < size) {
        uint8_t runValue = input[i];
        uint8_t runLength = 1;
        while (i + runLength < size && input[i + runLength] == runValue && runLength < 255) {
            runLength++;
        }
        compressed.push_back(runLength);
        compressed.push_back(runValue);
        i += runLength;
    }
    return compressed;
}

bool SegmWriter::Open(const std::string& filepath, uint16_t width, uint16_t height) {
    m_file.open(filepath, std::ios::binary | std::ios::out);
    if (!m_file.is_open()) return false;

    m_header.width = width;
    m_header.height = height;
    m_header.payloadOffset = sizeof(SegmHeader);

    m_file.write(reinterpret_cast<const char*>(&m_header), sizeof(SegmHeader));
    m_jpegCompressor = tjInitCompress();
    return true;
}

void SegmWriter::WriteFrame(uint64_t frameIndex, uint64_t timestamp, const std::vector<uint8_t>& colorBGR, const std::vector<uint8_t>& rawStencil) {
    if (!m_file.is_open() || !m_jpegCompressor) return;

    // 1. Compress Color Frame using TurboJPEG (Quality 85, Fast DCT)
    unsigned long jpegSize = 0;
    unsigned char* jpegBuf = nullptr;
    tjCompress2(m_jpegCompressor, colorBGR.data(), m_header.width, 0, m_header.height,
        TJPF_BGR, &jpegBuf, &jpegSize, TJSAMP_420, 85, TJFLAG_FASTDCT);

    // 2. Compress Stencil Mask using RLE
    std::vector<uint8_t> compressedStencil = CompressRLE(rawStencil.data(), rawStencil.size());

    // 3. Write Packet Header & Payloads
    FramePacketHeader pkt;
    pkt.frameIndex = frameIndex;
    pkt.timestampMicroseconds = timestamp;
    pkt.colorCompSize = static_cast<uint32_t>(jpegSize);
    pkt.stencilCompSize = static_cast<uint32_t>(compressedStencil.size());
    pkt.stencilUncompSize = static_cast<uint32_t>(rawStencil.size());

    m_file.write(reinterpret_cast<const char*>(&pkt), sizeof(FramePacketHeader));
    if (jpegBuf && jpegSize > 0) {
        m_file.write(reinterpret_cast<const char*>(jpegBuf), jpegSize);
        tjFree(jpegBuf);
    }
    if (!compressedStencil.empty()) {
        m_file.write(reinterpret_cast<const char*>(compressedStencil.data()), compressedStencil.size());
    }
    m_frameCount++;
}

void SegmWriter::Close() {
    if (!m_file.is_open()) return;

    m_header.totalFrames = m_frameCount;
    m_file.seekp(0, std::ios::beg);
    m_file.write(reinterpret_cast<const char*>(&m_header), sizeof(SegmHeader));
    m_file.close();

    if (m_jpegCompressor) {
        tjDestroy(m_jpegCompressor);
        m_jpegCompressor = nullptr;
    }
}