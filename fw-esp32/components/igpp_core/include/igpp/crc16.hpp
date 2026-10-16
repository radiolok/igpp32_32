#pragma once

#include <cstddef>
#include <cstdint>

namespace igpp {

// CRC-16/CCITT-FALSE: poly 0x1021, init 0xFFFF, no reflection, no xorout.
// Check value for "123456789" is 0x29B1.
uint16_t crc16(const uint8_t* data, size_t len, uint16_t crc = 0xFFFF);

}  // namespace igpp
