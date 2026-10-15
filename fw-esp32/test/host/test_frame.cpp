#include <set>

#include "igpp/frame.hpp"
#include "test.hpp"

using namespace igpp;

TEST(frame_geometry) {
    CHECK_EQ(kWidth, 96);
    CHECK_EQ(kHeight, 96);
    CHECK_EQ(kColumnBytes, 12);
    CHECK_EQ(kFrameBytes, size_t(1152));
}

// Same values as realCathodePos() for PANEL_ROWS == 3 in the MSP430 firmware.
TEST(chain_to_column_matches_msp430) {
    CHECK_EQ(frame_column_for_chain(0), 64);
    CHECK_EQ(frame_column_for_chain(31), 95);
    CHECK_EQ(frame_column_for_chain(32), 32);
    CHECK_EQ(frame_column_for_chain(63), 63);
    CHECK_EQ(frame_column_for_chain(64), 0);
    CHECK_EQ(frame_column_for_chain(95), 31);
    std::set<int> seen;
    for (int p = 0; p < kWidth; ++p) {
        seen.insert(frame_column_for_chain(p));
    }
    CHECK_EQ(seen.size(), size_t(kWidth));
}

// Layout of igpp.py ConvertImage(): column-major, rows packed MSB first.
TEST(pixel_layout_matches_igpp_py) {
    Frame f = blank_frame();
    set_pixel(f, 0, 0, true);
    CHECK_EQ(f[0], 0x7F);
    set_pixel(f, 1, 9, true);  // column 1 -> byte 12 + 1, row 9 -> bit 6
    CHECK_EQ(f[13], 0xBF);
    set_pixel(f, 95, 95, true);
    CHECK_EQ(f[kFrameBytes - 1], 0xFE);
    CHECK(get_pixel(f, 0, 0));
    CHECK(get_pixel(f, 1, 9));
    CHECK(!get_pixel(f, 1, 8));
    set_pixel(f, 0, 0, false);
    CHECK_EQ(f[0], 0xFF);
}

TEST(blank_column_is_all_dark) {
    for (auto b : kBlankColumn) {
        CHECK_EQ(b, 0xFF);
    }
}
