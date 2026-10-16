// ESP32-S3-DevKitC-1 N16R8 pin map, see docs/img/esp32-wiring.svg.
// OE of both blocks is hard-wired to +3.3V and is NOT driven by the MCU.
// Both MR lines have 10k pull-downs, so the registers stay cleared while the
// MCU boots.
#pragma once

#include "driver/gpio.h"

namespace igpp::board {

// Up block — anodes, SPI2 (through the GPIO matrix)
constexpr gpio_num_t kAnodeDi = GPIO_NUM_10;     // SPI2 MOSI
constexpr gpio_num_t kAnodeSrclk = GPIO_NUM_13;  // SPI2 CLK
constexpr gpio_num_t kAnodeLatch = GPIO_NUM_12;
constexpr gpio_num_t kAnodeMr = GPIO_NUM_14;     // active low

// Down block — cathodes, plain GPIO
constexpr gpio_num_t kCathDi = GPIO_NUM_4;
constexpr gpio_num_t kCathLatch = GPIO_NUM_6;
constexpr gpio_num_t kCathSrclk = GPIO_NUM_7;
constexpr gpio_num_t kCathMr = GPIO_NUM_15;      // active low

// microSD module, SPI3
constexpr gpio_num_t kSdSck = GPIO_NUM_39;
constexpr gpio_num_t kSdMosi = GPIO_NUM_40;
constexpr gpio_num_t kSdMiso = GPIO_NUM_41;
constexpr gpio_num_t kSdCs = GPIO_NUM_42;

// BOOT button on the DevKit: "next animation"; held at power-up: test mode.
constexpr gpio_num_t kButton = GPIO_NUM_0;

// All scan outputs are below GPIO32, so single-register set/clear works.
static_assert(kAnodeLatch < 32 && kAnodeMr < 32 && kCathDi < 32 && kCathLatch < 32 &&
                  kCathSrclk < 32 && kCathMr < 32,
              "scan GPIOs must be in GPIO_OUT_REG");

constexpr uint32_t bit(gpio_num_t n) { return 1u << unsigned(n); }

}  // namespace igpp::board
