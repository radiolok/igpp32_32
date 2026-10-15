#include <cstring>
#include <string>
#include <vector>

#include "igpp/scanner.hpp"
#include "test.hpp"

using namespace igpp;

namespace {

struct Event {
    std::string op;
    Column data{};
};

struct RecordingPort {
    std::vector<Event> ev;
    void anode_clear() { ev.push_back({"anode_clear"}); }
    void anode_send(const uint8_t* col) {
        Event e{"anode_send"};
        std::memcpy(e.data.data(), col, kColumnBytes);
        ev.push_back(e);
    }
    void cathode_start() { ev.push_back({"cathode_start"}); }
    void cathode_clock() { ev.push_back({"cathode_clock"}); }
    void latch_all() { ev.push_back({"latch_all"}); }
    void cathode_off() { ev.push_back({"cathode_off"}); }
};

Frame numbered_frame() {
    // Every column carries its own index, so the sent column is identifiable.
    Frame f;
    for (int x = 0; x < kWidth; ++x) {
        for (int b = 0; b < kColumnBytes; ++b) {
            f[size_t(x) * kColumnBytes + b] = uint8_t(x);
        }
    }
    return f;
}

}  // namespace

// Sequence of the MSP430 firmware: blank, move cathode, latch, data, latch.
TEST(scanner_column_sequence) {
    RecordingPort port;
    Scanner<RecordingPort> s(port);
    s.step();
    const char* expect_first[] = {"anode_clear", "anode_send", "cathode_start", "latch_all",
                                  "anode_send", "latch_all"};
    CHECK_EQ(port.ev.size(), size_t(6));
    for (size_t i = 0; i < 6 && i < port.ev.size(); ++i) {
        CHECK(port.ev[i].op == expect_first[i]);
    }
    CHECK(port.ev[1].data == kBlankColumn);
    CHECK_EQ(s.chain_pos(), 0);

    port.ev.clear();
    s.step();
    const char* expect_next[] = {"anode_clear", "anode_send", "cathode_clock", "latch_all",
                                 "anode_send", "latch_all"};
    for (size_t i = 0; i < 6 && i < port.ev.size(); ++i) {
        CHECK(port.ev[i].op == expect_next[i]);
    }
    CHECK_EQ(s.chain_pos(), 1);
}

TEST(scanner_sends_columns_in_chain_order) {
    RecordingPort port;
    Scanner<RecordingPort> s(port);
    s.back() = numbered_frame();
    s.publish();
    for (int p = 0; p < kWidth; ++p) {
        port.ev.clear();
        s.step();
        CHECK_EQ(int(port.ev[4].data[0]), frame_column_for_chain(p));
    }
    CHECK_EQ(s.frames(), 1u);
    // Exactly one cathode_start per frame.
    port.ev.clear();
    for (int p = 0; p < kWidth; ++p) {
        s.step();
    }
    int starts = 0;
    for (const auto& e : port.ev) {
        starts += e.op == "cathode_start";
    }
    CHECK_EQ(starts, 1);
    CHECK_EQ(s.frames(), 2u);
}

TEST(scanner_swaps_only_at_frame_start) {
    RecordingPort port;
    Scanner<RecordingPort> s(port);
    for (int p = 0; p < 10; ++p) {
        s.step();  // into the middle of frame 1, front is blank
    }
    s.back() = numbered_frame();
    s.publish();
    CHECK(s.swap_pending());
    for (int p = 10; p < kWidth; ++p) {
        port.ev.clear();
        s.step();
        CHECK(port.ev[4].data == kBlankColumn);  // still the old frame
    }
    CHECK(s.swap_pending());
    port.ev.clear();
    s.step();  // frame 2 begins: the new frame is visible
    CHECK(!s.swap_pending());
    CHECK_EQ(int(port.ev[4].data[0]), frame_column_for_chain(0));
}

TEST(scanner_cathode_off) {
    RecordingPort port;
    Scanner<RecordingPort> s(port);
    s.cathode_off();
    CHECK_EQ(port.ev.size(), size_t(1));
    CHECK(port.ev[0].op == "cathode_off");
}
