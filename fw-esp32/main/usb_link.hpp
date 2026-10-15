// Host link over the native USB port (USB-Serial-JTAG CDC).
#pragma once

#include <cstdint>

namespace usb_link {

void start();

uint32_t rx_frames();
uint32_t rx_dropped();

}  // namespace usb_link
