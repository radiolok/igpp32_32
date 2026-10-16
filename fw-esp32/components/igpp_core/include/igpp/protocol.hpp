// Host link protocol (USB CDC).
//
// Two formats share one byte stream:
//
// * legacy — raw 1152-byte frames, exactly what igpp.py sends today.
//   Frames are delimited by time: if the stream pauses for more than
//   kResyncGapMs in the middle of a frame, the partial frame is dropped and
//   the next byte starts a new one. This recovers from lost bytes, which the
//   MSP430 version could not do.
//
// * v2 packets:
//     "IGP2" | type:u8 | len:u16 LE | payload[len] | crc16:u16 LE
//   crc16 (CCITT-FALSE) covers type, len and payload. Replies from the device
//   use the same format with type | 0x80.
//
// A packet is recognised by its 4-byte magic at the start of a frame slot, so
// a legacy frame that begins with the bytes "IGP2" would be misread; with
// real pictures this does not happen in practice.
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "igpp/frame.hpp"

namespace igpp::proto {

constexpr uint8_t kMagic[4] = {'I', 'G', 'P', '2'};
constexpr uint32_t kResyncGapMs = 10;
constexpr uint16_t kMaxPayload = 2048;
constexpr uint8_t kReplyFlag = 0x80;

enum class Type : uint8_t {
    Frame = 0x01,      // payload: 1152-byte frame, shown immediately (live mode)
    Play = 0x02,       // payload: u16 index in the playlist, 0xFFFF = next
    Stop = 0x03,       // blank the display and pause the player
    Resume = 0x04,     // leave live/stop mode, continue the playlist
    Status = 0x05,     // reply: StatusReply
    Ping = 0x06,       // reply: same payload
    List = 0x07,       // reply: names of playlist items, '\n'-separated
    FileOpen = 0x10,   // payload: file name (no path), creates/truncates on storage
    FileData = 0x11,   // payload: next chunk of the file
    FileClose = 0x12,  // finish the upload, rescan the playlist
};

// Reply payload for Type::Status (little endian).
struct __attribute__((packed)) StatusReply {
    uint32_t scan_frames;    // frames scanned since boot
    uint32_t shown_frames;   // frames published to the scanner
    uint32_t rx_frames;      // frames received from the host
    uint32_t rx_dropped;     // partial frames / bad packets dropped
    uint16_t playlist_size;
    uint16_t playlist_index;
    uint8_t mode;            // 0 playlist, 1 live, 2 stopped
    uint8_t storage;         // bit0 SD mounted, bit1 flash mounted
};

class Handler {
public:
    virtual ~Handler() = default;
    virtual void on_frame(const Frame& frame) = 0;
    virtual void on_packet(Type type, const uint8_t* payload, uint16_t len) = 0;
};

class StreamParser {
public:
    explicit StreamParser(Handler& handler) : handler_(handler) {}

    // Feed received bytes; `now_ms` is a monotonic time in milliseconds.
    void feed(const uint8_t* data, size_t len, uint32_t now_ms);

    uint32_t frames() const { return frames_; }
    uint32_t packets() const { return packets_; }
    uint32_t dropped() const { return dropped_; }

private:
    enum class State { Idle, Detect, Legacy, Header, Payload, Crc };

    void reset();
    void drop();
    void legacy_byte(uint8_t b);

    Handler& handler_;
    State state_ = State::Idle;
    Frame frame_{};
    size_t pos_ = 0;
    uint8_t detect_[4] = {};
    size_t detect_len_ = 0;
    uint8_t header_[3] = {};  // type, len lo, len hi
    size_t header_pos_ = 0;
    uint8_t type_ = 0;
    uint16_t len_ = 0;
    std::vector<uint8_t> payload_;
    uint8_t crc_[2] = {};
    size_t crc_pos_ = 0;
    uint32_t last_ms_ = 0;
    bool have_last_ = false;
    uint32_t frames_ = 0;
    uint32_t packets_ = 0;
    uint32_t dropped_ = 0;
};

// Build a v2 packet (used by the device for replies and by host tests).
std::vector<uint8_t> make_packet(uint8_t type, const uint8_t* payload, uint16_t len);

}  // namespace igpp::proto
