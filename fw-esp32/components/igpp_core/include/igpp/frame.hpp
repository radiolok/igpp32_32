// Frame format of the 96x96 IGPP-32/32 display.
//
// Layout is the one used by the MSP430 firmware and igpp.py:
//   * column-major, 12 bytes per column, 96 columns -> 1152 bytes;
//   * inside a column byte k holds rows 8k..8k+7, MSB is the top row;
//   * anodes are active-low: bit 0 = dot is lit, bit 1 = dark.
// The frame is sent to the anode chain as is, so no conversion happens in the
// scan path.
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace igpp {

constexpr int kPanelSize = 32;
constexpr int kPanelRows = 3;
constexpr int kPanelCols = 3;
constexpr int kWidth = kPanelSize * kPanelCols;    // 96 columns (cathodes)
constexpr int kHeight = kPanelSize * kPanelRows;   // 96 rows (anodes)
constexpr int kColumnBytes = kHeight / 8;          // 12
constexpr size_t kFrameBytes = size_t(kWidth) * kColumnBytes;  // 1152

using Frame = std::array<uint8_t, kFrameBytes>;
using Column = std::array<uint8_t, kColumnBytes>;

// Column of anode data that switches every anode off.
constexpr Column kBlankColumn = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Frame column shown while the cathode chain is at position `chain_pos`.
// The cathode chain runs through the panel columns in reverse order
// (realCathodePos() in the MSP430 firmware).
constexpr int frame_column_for_chain(int chain_pos) {
    return (64 - (chain_pos & 0xE0)) + (chain_pos & 0x1F);
}

inline const uint8_t* column_ptr(const Frame& f, int column) {
    return f.data() + size_t(column) * kColumnBytes;
}

inline bool get_pixel(const Frame& f, int x, int y) {
    const uint8_t b = f[size_t(x) * kColumnBytes + (y >> 3)];
    return ((b >> (7 - (y & 7))) & 1) == 0;  // 0 = lit
}

inline void set_pixel(Frame& f, int x, int y, bool lit) {
    uint8_t& b = f[size_t(x) * kColumnBytes + (y >> 3)];
    const uint8_t mask = uint8_t(0x80 >> (y & 7));
    if (lit) {
        b = uint8_t(b & ~mask);
    } else {
        b = uint8_t(b | mask);
    }
}

inline void fill(Frame& f, bool lit) { f.fill(lit ? 0x00 : 0xFF); }

inline Frame blank_frame() {
    Frame f;
    fill(f, false);
    return f;
}

}  // namespace igpp
