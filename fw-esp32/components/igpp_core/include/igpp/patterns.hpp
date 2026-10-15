// Built-in content: test patterns for the bench and the pictures that lived in
// the MSP430 firmware. Shown when no SD card / flash content is available.
#pragma once

#include <cstddef>
#include <cstdint>

#include "igpp/frame.hpp"

namespace igpp::patterns {

struct Pattern {
    const char* name;
    uint32_t frame_count;
    uint16_t frame_ms;
    void (*render)(uint32_t index, Frame& out);
};

// All built-in patterns, in playlist order.
const Pattern* all(size_t& count);

// Find by name, nullptr if absent.
const Pattern* find(const char* name);

// The two pictures from frameBuffer[] of the MSP430 firmware.
extern const uint8_t kMspPicture[2][kFrameBytes];

}  // namespace igpp::patterns
