// .igv animation container.
//
//   offset size
//   0      4   magic "IGV1"
//   4      2   version (1)
//   6      2   width  (96)
//   8      2   height (96)
//   10     2   frame size in bytes (1152)
//   12     4   frame count
//   16     2   frame duration, ms
//   18     2   flags (bit 0: loop)
//   20     12  reserved, zero
//   32     ... frames, frame_size bytes each, in the native panel layout
//
// All fields are little endian. The player copies frames straight into the
// scan buffer, so the file layout equals igpp::Frame.
#pragma once

#include <cstddef>
#include <cstdint>

#include "igpp/frame.hpp"

namespace igpp::igv {

constexpr size_t kHeaderSize = 32;
constexpr uint16_t kVersion = 1;
constexpr uint16_t kFlagLoop = 1;

struct Header {
    uint16_t version = kVersion;
    uint16_t width = kWidth;
    uint16_t height = kHeight;
    uint16_t frame_size = kFrameBytes;
    uint32_t frame_count = 0;
    uint16_t frame_ms = 40;
    uint16_t flags = kFlagLoop;
};

enum class Error { Ok, TooShort, BadMagic, BadVersion, BadGeometry, BadTiming };

// Parse and validate a 32-byte header.
Error parse_header(const uint8_t* data, size_t len, Header& out);

// Serialise a header into 32 bytes.
void write_header(const Header& h, uint8_t out[kHeaderSize]);

// Offset of frame `index` in the file.
constexpr size_t frame_offset(uint32_t index) {
    return kHeaderSize + size_t(index) * kFrameBytes;
}

const char* error_str(Error e);

}  // namespace igpp::igv
