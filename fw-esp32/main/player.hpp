// Playlist player: animations from storage, built-in patterns, live frames.
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "igpp/frame.hpp"

namespace player {

enum class Mode : uint8_t { Playlist = 0, Live = 1, Stopped = 2 };

// test_mode: play only the built-in test patterns.
void start(bool test_mode);

void next();
void play(uint16_t index);
void stop();
void resume();
void rescan();

// A frame from the host: switches to live mode until the stream pauses.
void live_frame(const igpp::Frame& frame);

Mode mode();
size_t size();
size_t index();
std::string list();

}  // namespace player
