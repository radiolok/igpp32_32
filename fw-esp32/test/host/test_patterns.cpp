#include <cstring>

#include "igpp/patterns.hpp"
#include "test.hpp"

using namespace igpp;

TEST(patterns_render_every_frame) {
    size_t n = 0;
    const auto* p = patterns::all(n);
    CHECK(n >= 5);
    for (size_t i = 0; i < n; ++i) {
        CHECK(p[i].frame_count > 0);
        CHECK(p[i].frame_ms > 0);
        Frame f;
        for (uint32_t k = 0; k < p[i].frame_count; ++k) {
            p[i].render(k, f);
        }
    }
}

TEST(patterns_column_walk_lights_one_column) {
    const auto* p = patterns::find("test-columns");
    CHECK(p != nullptr);
    if (!p) return;
    Frame f;
    p->render(5, f);
    for (int x = 0; x < kWidth; ++x) {
        for (int y = 0; y < kHeight; ++y) {
            CHECK_EQ(get_pixel(f, x, y), x == 5);
        }
    }
}

TEST(msp_pictures_are_inverse) {
    // frameBuffer[1] is the inverse of frameBuffer[0] in the MSP430 firmware.
    for (size_t i = 0; i < kFrameBytes; ++i) {
        CHECK_EQ(uint8_t(patterns::kMspPicture[0][i] ^ patterns::kMspPicture[1][i]), 0xFF);
    }
}
