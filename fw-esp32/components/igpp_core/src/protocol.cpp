#include "igpp/protocol.hpp"

#include <cstring>

#include "igpp/crc16.hpp"

namespace igpp::proto {

void StreamParser::reset() {
    state_ = State::Idle;
    pos_ = 0;
    detect_len_ = 0;
    header_pos_ = 0;
    crc_pos_ = 0;
    payload_.clear();
}

void StreamParser::drop() {
    ++dropped_;
    reset();
}

void StreamParser::legacy_byte(uint8_t b) {
    frame_[pos_++] = b;
    if (pos_ == kFrameBytes) {
        ++frames_;
        handler_.on_frame(frame_);
        reset();
    }
}

void StreamParser::feed(const uint8_t* data, size_t len, uint32_t now_ms) {
    if (len == 0) {
        return;
    }
    if (have_last_ && state_ != State::Idle && uint32_t(now_ms - last_ms_) > kResyncGapMs) {
        drop();
    }
    have_last_ = true;
    last_ms_ = now_ms;

    for (size_t i = 0; i < len; ++i) {
        const uint8_t b = data[i];
        switch (state_) {
        case State::Idle:
            state_ = State::Detect;
            detect_len_ = 0;
            [[fallthrough]];
        case State::Detect:
            detect_[detect_len_++] = b;
            if (b != kMagic[detect_len_ - 1]) {
                // Not a packet: everything collected so far is frame data.
                state_ = State::Legacy;
                pos_ = 0;
                const size_t n = detect_len_;
                detect_len_ = 0;
                for (size_t k = 0; k < n && state_ == State::Legacy; ++k) {
                    legacy_byte(detect_[k]);
                }
            } else if (detect_len_ == sizeof(kMagic)) {
                state_ = State::Header;
                header_pos_ = 0;
            }
            break;
        case State::Legacy:
            legacy_byte(b);
            break;
        case State::Header:
            header_[header_pos_++] = b;
            if (header_pos_ == sizeof(header_)) {
                type_ = header_[0];
                len_ = uint16_t(header_[1] | (header_[2] << 8));
                if (len_ > kMaxPayload) {
                    drop();
                    break;
                }
                payload_.clear();
                payload_.reserve(len_);
                state_ = len_ ? State::Payload : State::Crc;
                crc_pos_ = 0;
            }
            break;
        case State::Payload:
            payload_.push_back(b);
            if (payload_.size() == len_) {
                state_ = State::Crc;
                crc_pos_ = 0;
            }
            break;
        case State::Crc: {
            crc_[crc_pos_++] = b;
            if (crc_pos_ < 2) {
                break;
            }
            uint16_t crc = crc16(header_, sizeof(header_));
            crc = crc16(payload_.data(), payload_.size(), crc);
            const uint16_t got = uint16_t(crc_[0] | (crc_[1] << 8));
            if (crc != got) {
                drop();
                break;
            }
            ++packets_;
            if (type_ == uint8_t(Type::Frame)) {
                if (len_ == kFrameBytes) {
                    std::memcpy(frame_.data(), payload_.data(), kFrameBytes);
                    ++frames_;
                    handler_.on_frame(frame_);
                } else {
                    ++dropped_;
                }
            } else {
                handler_.on_packet(Type(type_), payload_.data(), len_);
            }
            reset();
            break;
        }
        }
    }
}

std::vector<uint8_t> make_packet(uint8_t type, const uint8_t* payload, uint16_t len) {
    std::vector<uint8_t> out;
    out.reserve(sizeof(kMagic) + 3 + len + 2);
    out.insert(out.end(), kMagic, kMagic + sizeof(kMagic));
    const uint8_t header[3] = {type, uint8_t(len & 0xFF), uint8_t(len >> 8)};
    out.insert(out.end(), header, header + 3);
    if (len) {
        out.insert(out.end(), payload, payload + len);
    }
    uint16_t crc = crc16(header, 3);
    crc = crc16(payload, len, crc);
    out.push_back(uint8_t(crc & 0xFF));
    out.push_back(uint8_t(crc >> 8));
    return out;
}

}  // namespace igpp::proto
