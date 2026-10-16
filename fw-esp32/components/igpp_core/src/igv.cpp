#include "igpp/igv.hpp"

#include <cstring>

namespace igpp::igv {

namespace {

uint16_t rd16(const uint8_t* p) { return uint16_t(p[0] | (p[1] << 8)); }
uint32_t rd32(const uint8_t* p) {
    return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}
void wr16(uint8_t* p, uint16_t v) {
    p[0] = uint8_t(v);
    p[1] = uint8_t(v >> 8);
}
void wr32(uint8_t* p, uint32_t v) {
    for (int i = 0; i < 4; ++i) {
        p[i] = uint8_t(v >> (8 * i));
    }
}

}  // namespace

Error parse_header(const uint8_t* d, size_t len, Header& h) {
    if (len < kHeaderSize) {
        return Error::TooShort;
    }
    if (std::memcmp(d, "IGV1", 4) != 0) {
        return Error::BadMagic;
    }
    h.version = rd16(d + 4);
    h.width = rd16(d + 6);
    h.height = rd16(d + 8);
    h.frame_size = rd16(d + 10);
    h.frame_count = rd32(d + 12);
    h.frame_ms = rd16(d + 16);
    h.flags = rd16(d + 18);
    if (h.version != kVersion) {
        return Error::BadVersion;
    }
    if (h.width != kWidth || h.height != kHeight || h.frame_size != kFrameBytes) {
        return Error::BadGeometry;
    }
    if (h.frame_ms == 0) {
        return Error::BadTiming;
    }
    return Error::Ok;
}

void write_header(const Header& h, uint8_t out[kHeaderSize]) {
    std::memset(out, 0, kHeaderSize);
    std::memcpy(out, "IGV1", 4);
    wr16(out + 4, h.version);
    wr16(out + 6, h.width);
    wr16(out + 8, h.height);
    wr16(out + 10, h.frame_size);
    wr32(out + 12, h.frame_count);
    wr16(out + 16, h.frame_ms);
    wr16(out + 18, h.flags);
}

const char* error_str(Error e) {
    switch (e) {
    case Error::Ok: return "ok";
    case Error::TooShort: return "file too short";
    case Error::BadMagic: return "not an IGV1 file";
    case Error::BadVersion: return "unsupported version";
    case Error::BadGeometry: return "wrong geometry (expected 96x96, 1152 bytes)";
    case Error::BadTiming: return "frame duration is zero";
    }
    return "?";
}

}  // namespace igpp::igv
