// Display driver: runs the column scan on CPU1 and accepts frames from tasks.
#pragma once

#include <cstdint>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "igpp/frame.hpp"

namespace igpp::display {

// Bring the registers into a safe state, set up SPI2 and start the scan
// interrupt on CPU1. Also starts the stall monitor on CPU0.
esp_err_t start();

// Copy `frame` into the back buffer and show it from the next frame start.
// Waits up to `wait` for the previous frame to be taken. Thread-safe.
bool submit(const Frame& frame, TickType_t wait);

// Show an all-dark frame.
void blank();

// Deselect all cathodes immediately and stop the scan (emergency).
void emergency_off();

uint32_t scan_frames();   // frames scanned since start
uint32_t shown_frames();  // frames accepted by submit()

}  // namespace igpp::display
