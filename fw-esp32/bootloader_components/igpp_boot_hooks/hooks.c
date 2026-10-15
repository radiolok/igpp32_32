// Second-stage bootloader hook: put the display registers into a safe state
// a few milliseconds after reset, long before the application starts.
//
// Safe state = cathode chain cleared and latched, i.e. no cathode selected,
// so no dot can be lit whatever the anodes hold. The MR lines also have 10k
// pull-downs; this hook adds the latch pulse that copies the cleared shift
// register to the outputs. Pins must match components/igpp_display/include/igpp/board.hpp.

#include <stdint.h>

#include "esp_rom_gpio.h"
#include "esp_rom_sys.h"
#include "soc/gpio_reg.h"
#include "soc/gpio_sig_map.h"
#include "soc/soc.h"

#define ANODE_LATCH 12
#define ANODE_MR 14
#define CATH_DI 4
#define CATH_LATCH 6
#define CATH_SRCLK 7
#define CATH_MR 15

void bootloader_hooks_include(void) {}

static void output_low(uint32_t gpio)
{
    esp_rom_gpio_pad_select_gpio(gpio);
    esp_rom_gpio_connect_out_signal(gpio, SIG_GPIO_OUT_IDX, false, false);
    REG_WRITE(GPIO_OUT_W1TC_REG, 1u << gpio);
    REG_WRITE(GPIO_ENABLE_W1TS_REG, 1u << gpio);
}

void bootloader_before_init(void)
{
    const uint32_t pins[] = {ANODE_MR, CATH_MR, ANODE_LATCH, CATH_LATCH, CATH_DI, CATH_SRCLK};
    for (unsigned i = 0; i < sizeof(pins) / sizeof(pins[0]); ++i) {
        output_low(pins[i]);  // MR low: shift registers cleared
    }
    esp_rom_delay_us(1);
    REG_WRITE(GPIO_OUT_W1TS_REG, (1u << ANODE_LATCH) | (1u << CATH_LATCH));
    esp_rom_delay_us(1);
    REG_WRITE(GPIO_OUT_W1TC_REG, (1u << ANODE_LATCH) | (1u << CATH_LATCH));
    // MR stays low until the application takes over.
}
