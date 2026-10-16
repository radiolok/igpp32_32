#include <cstring>
#include <string>
#include <vector>

#include "igpp/crc16.hpp"
#include "igpp/protocol.hpp"
#include "test.hpp"

using namespace igpp;
using namespace igpp::proto;

namespace {

struct Recorder : Handler {
    std::vector<Frame> frames;
    std::vector<std::pair<Type, std::vector<uint8_t>>> packets;
    void on_frame(const Frame& f) override { frames.push_back(f); }
    void on_packet(Type type, const uint8_t* p, uint16_t len) override {
        packets.push_back({type, std::vector<uint8_t>(p, p + len)});
    }
};

Frame pattern_frame(uint8_t seed) {
    Frame f;
    for (size_t i = 0; i < kFrameBytes; ++i) {
        f[i] = uint8_t(i * 7 + seed);
    }
    return f;
}

void feed_chunks(StreamParser& p, const uint8_t* d, size_t n, uint32_t t, size_t chunk = 64) {
    for (size_t i = 0; i < n; i += chunk) {
        p.feed(d + i, std::min(chunk, n - i), t);
    }
}

}  // namespace

TEST(crc16_check_value) {
    const char* s = "123456789";
    CHECK_EQ(crc16(reinterpret_cast<const uint8_t*>(s), 9), 0x29B1);
}

TEST(legacy_frames_in_usb_chunks) {
    Recorder r;
    StreamParser p(r);
    for (uint8_t k = 0; k < 3; ++k) {
        const Frame f = pattern_frame(k);
        feed_chunks(p, f.data(), f.size(), 40u * k);  // igpp.py: one frame every ~40 ms
    }
    CHECK_EQ(r.frames.size(), size_t(3));
    CHECK(r.frames.size() == 3 && r.frames[2] == pattern_frame(2));
    CHECK_EQ(p.dropped(), 0u);
}

TEST(legacy_resync_after_lost_bytes) {
    Recorder r;
    StreamParser p(r);
    Frame f = pattern_frame(1);
    // A frame with 10 bytes lost on the way...
    feed_chunks(p, f.data(), f.size() - 10, 0);
    // ...then the next frame arrives after the usual pause.
    const Frame g = pattern_frame(2);
    feed_chunks(p, g.data(), g.size(), 40);
    CHECK_EQ(p.dropped(), 1u);
    CHECK_EQ(r.frames.size(), size_t(1));
    CHECK(r.frames.size() == 1 && r.frames[0] == g);
}

TEST(packet_frame_and_commands) {
    Recorder r;
    StreamParser p(r);
    const Frame f = pattern_frame(5);
    auto pkt = make_packet(uint8_t(Type::Frame), f.data(), uint16_t(f.size()));
    feed_chunks(p, pkt.data(), pkt.size(), 0);
    const uint8_t idx[2] = {3, 0};
    auto play = make_packet(uint8_t(Type::Play), idx, 2);
    p.feed(play.data(), play.size(), 1);
    auto stop = make_packet(uint8_t(Type::Stop), nullptr, 0);
    p.feed(stop.data(), stop.size(), 2);

    CHECK_EQ(r.frames.size(), size_t(1));
    CHECK(r.frames.size() == 1 && r.frames[0] == f);
    CHECK_EQ(r.packets.size(), size_t(2));
    CHECK(r.packets.size() == 2 && r.packets[0].first == Type::Play);
    CHECK(r.packets.size() == 2 && r.packets[0].second == std::vector<uint8_t>({3, 0}));
    CHECK(r.packets.size() == 2 && r.packets[1].first == Type::Stop);
    CHECK_EQ(p.dropped(), 0u);
}

TEST(packet_bad_crc_is_dropped) {
    Recorder r;
    StreamParser p(r);
    auto ping = make_packet(uint8_t(Type::Ping), reinterpret_cast<const uint8_t*>("hi"), 2);
    ping.back() ^= 0xFF;
    p.feed(ping.data(), ping.size(), 0);
    CHECK_EQ(r.packets.size(), size_t(0));
    CHECK_EQ(p.dropped(), 1u);
    // The parser is usable right away.
    auto ok = make_packet(uint8_t(Type::Ping), reinterpret_cast<const uint8_t*>("hi"), 2);
    p.feed(ok.data(), ok.size(), 1);
    CHECK_EQ(r.packets.size(), size_t(1));
}

TEST(legacy_frame_starting_like_magic) {
    // "IGx..." is not the full magic, so it is frame data.
    Recorder r;
    StreamParser p(r);
    Frame f = pattern_frame(9);
    f[0] = 'I';
    f[1] = 'G';
    f[2] = 'x';
    feed_chunks(p, f.data(), f.size(), 0);
    CHECK_EQ(r.frames.size(), size_t(1));
    CHECK(r.frames.size() == 1 && r.frames[0] == f);
}

TEST(mixed_legacy_and_packets) {
    Recorder r;
    StreamParser p(r);
    const Frame a = pattern_frame(1);
    feed_chunks(p, a.data(), a.size(), 0);
    auto ping = make_packet(uint8_t(Type::Ping), nullptr, 0);
    p.feed(ping.data(), ping.size(), 5);
    const Frame b = pattern_frame(2);
    feed_chunks(p, b.data(), b.size(), 6);
    CHECK_EQ(r.frames.size(), size_t(2));
    CHECK_EQ(r.packets.size(), size_t(1));
}

TEST(oversized_packet_is_dropped) {
    Recorder r;
    StreamParser p(r);
    const uint8_t hdr[] = {'I', 'G', 'P', '2', 0x11, 0xFF, 0xFF};
    p.feed(hdr, sizeof(hdr), 0);
    CHECK_EQ(p.dropped(), 1u);
}
