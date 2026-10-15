#include <cstring>

#include "igpp/igv.hpp"
#include "test.hpp"

using namespace igpp;
using namespace igpp::igv;

TEST(igv_header_roundtrip) {
    Header h;
    h.frame_count = 1234;
    h.frame_ms = 33;
    h.flags = kFlagLoop;
    uint8_t buf[kHeaderSize];
    write_header(h, buf);
    CHECK(std::memcmp(buf, "IGV1", 4) == 0);
    Header g;
    CHECK(parse_header(buf, sizeof(buf), g) == Error::Ok);
    CHECK_EQ(g.frame_count, 1234u);
    CHECK_EQ(g.frame_ms, 33);
    CHECK_EQ(g.flags, kFlagLoop);
    CHECK_EQ(g.frame_size, kFrameBytes);
}

// Byte-exact layout, shared with tools/igv.py.
TEST(igv_header_layout) {
    Header h;
    h.frame_count = 0x01020304;
    h.frame_ms = 40;
    uint8_t buf[kHeaderSize];
    write_header(h, buf);
    const uint8_t expect[20] = {'I', 'G', 'V', '1', 1, 0, 96, 0, 96, 0, 0x80, 0x04,
                                0x04, 0x03, 0x02, 0x01, 40, 0, 1, 0};
    CHECK(std::memcmp(buf, expect, sizeof(expect)) == 0);
    for (size_t i = 20; i < kHeaderSize; ++i) {
        CHECK_EQ(buf[i], 0);
    }
}

TEST(igv_header_errors) {
    uint8_t buf[kHeaderSize];
    Header h, g;
    write_header(h, buf);
    CHECK(parse_header(buf, 10, g) == Error::TooShort);
    buf[0] = 'X';
    CHECK(parse_header(buf, sizeof(buf), g) == Error::BadMagic);
    write_header(h, buf);
    buf[6] = 64;
    CHECK(parse_header(buf, sizeof(buf), g) == Error::BadGeometry);
    write_header(h, buf);
    buf[16] = buf[17] = 0;
    CHECK(parse_header(buf, sizeof(buf), g) == Error::BadTiming);
}

TEST(igv_frame_offset) {
    CHECK_EQ(frame_offset(0), size_t(32));
    CHECK_EQ(frame_offset(2), size_t(32 + 2 * 1152));
}
