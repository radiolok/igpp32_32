// Column scan on CPU1.
//
// The whole column sequence runs inside an IRAM-safe GPTimer interrupt pinned
// to CPU1. That interrupt keeps running while the flash cache is disabled
// (FAT writes, NVS), so no column is ever left lit for longer than one period.
// SPI2 is configured once through the driver and then driven directly through
// its registers from the interrupt: 12 bytes fit into the SPI data buffer, no
// DMA is used.

#include "igpp/display.hpp"

#include <atomic>
#include <cstring>

#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "driver/spi_master.h"
#include "esp_attr.h"
#include "esp_check.h"
#include "esp_rom_sys.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "hal/spi_ll.h"
#include "igpp/board.hpp"
#include "igpp/scanner.hpp"
#include "sdkconfig.h"
#include "soc/gpio_reg.h"
#include "soc/spi_struct.h"

namespace igpp::display {

namespace {

constexpr const char* TAG = "display";
using namespace igpp::board;

IGPP_ALWAYS_INLINE void set(uint32_t mask) { REG_WRITE(GPIO_OUT_W1TS_REG, mask); }
IGPP_ALWAYS_INLINE void clr(uint32_t mask) { REG_WRITE(GPIO_OUT_W1TC_REG, mask); }

// Minimum pulse width for the shift registers: ~50 ns at 240 MHz.
IGPP_ALWAYS_INLINE void hold() {
    __asm__ __volatile__("nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop;");
}

struct HwPort {
    IGPP_ALWAYS_INLINE void anode_clear() {
        clr(bit(kAnodeMr));
        hold();
        set(bit(kAnodeMr));
    }

    // Shift 12 bytes into the anode chain; returns when the last bit is out
    // (the MSP430 polled UCBUSY for the same reason).
    IGPP_ALWAYS_INLINE void anode_send(const uint8_t* column) {
        spi_dev_t* hw = &GPSPI2;
        spi_ll_clear_int_stat(hw);
        spi_ll_write_buffer(hw, column, kColumnBytes * 8);
        spi_ll_set_mosi_bitlen(hw, kColumnBytes * 8);
        spi_ll_apply_config(hw);
        spi_ll_user_start(hw);
        while (!spi_ll_usr_is_done(hw)) {
        }
    }

    // Column 0: clear the cathode chain and shift a single 1 in.
    IGPP_ALWAYS_INLINE void cathode_start() {
        clr(bit(kCathMr));
        hold();
        set(bit(kCathMr));
        set(bit(kCathDi));
        hold();
        set(bit(kCathSrclk));
        hold();
        clr(bit(kCathSrclk) | bit(kCathDi));
    }

    IGPP_ALWAYS_INLINE void cathode_clock() {
        set(bit(kCathSrclk));
        hold();
        clr(bit(kCathSrclk));
    }

    IGPP_ALWAYS_INLINE void latch_all() {
        set(bit(kAnodeLatch) | bit(kCathLatch));
        hold();
        clr(bit(kAnodeLatch) | bit(kCathLatch));
    }

    // Clear the cathode chain and latch it: no cathode selected. MR is left
    // low on purpose, so nothing can be shifted in afterwards.
    IGPP_ALWAYS_INLINE void cathode_off() {
        clr(bit(kCathMr));
        hold();
        set(bit(kCathLatch));
        hold();
        clr(bit(kCathLatch));
    }
};

HwPort g_port;
Scanner<HwPort> g_scanner(g_port);
std::atomic<bool> g_stopped{false};
std::atomic<uint32_t> g_shown{0};

gptimer_handle_t g_timer = nullptr;
spi_device_handle_t g_spi = nullptr;
SemaphoreHandle_t g_submit_mutex = nullptr;

bool IRAM_ATTR on_column(gptimer_handle_t, const gptimer_alarm_event_data_t*, void*) {
    if (!g_stopped.load(std::memory_order_relaxed)) {
        g_scanner.step();
    }
    return false;
}

void configure_outputs() {
    const uint32_t outputs = bit(kAnodeLatch) | bit(kAnodeMr) | bit(kCathDi) | bit(kCathLatch) |
                             bit(kCathSrclk) | bit(kCathMr);
    // Levels first, then enable the drivers: MR stays low (registers cleared).
    clr(outputs);
    gpio_config_t io = {};
    io.pin_bit_mask = outputs;
    io.mode = GPIO_MODE_OUTPUT;
    io.pull_up_en = GPIO_PULLUP_DISABLE;
    io.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io.intr_type = GPIO_INTR_DISABLE;
    ESP_ERROR_CHECK(gpio_config(&io));

    // Latch the cleared shift registers: no cathode selected.
    g_port.latch_all();
    set(bit(kAnodeMr) | bit(kCathMr));
}

esp_err_t configure_spi() {
    spi_bus_config_t bus = {};
    bus.mosi_io_num = kAnodeDi;
    bus.miso_io_num = -1;
    bus.sclk_io_num = kAnodeSrclk;
    bus.quadwp_io_num = -1;
    bus.quadhd_io_num = -1;
    bus.max_transfer_sz = 64;
    ESP_RETURN_ON_ERROR(spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_DISABLED), TAG, "spi bus");

    spi_device_interface_config_t dev = {};
    dev.mode = 0;  // MSP430: UCCKPH=1, UCCKPL=0
    dev.clock_speed_hz = CONFIG_IGPP_SPI_CLOCK_HZ;
    dev.spics_io_num = -1;
    dev.queue_size = 1;
    ESP_RETURN_ON_ERROR(spi_bus_add_device(SPI2_HOST, &dev, &g_spi), TAG, "spi device");

    // One driver transaction programs clock, mode and bit length; then the bus
    // is taken for good and only the scan interrupt touches SPI2.
    static const uint8_t blank[kColumnBytes] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                                0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    spi_transaction_t t = {};
    t.length = kColumnBytes * 8;
    t.tx_buffer = blank;
    ESP_RETURN_ON_ERROR(spi_device_polling_transmit(g_spi, &t), TAG, "spi first transfer");
    ESP_RETURN_ON_ERROR(spi_device_acquire_bus(g_spi, portMAX_DELAY), TAG, "spi acquire");

    int khz = 0;
    spi_device_get_actual_freq(g_spi, &khz);
    ESP_LOGI(TAG, "SPI2 anode clock %d kHz", khz);
    return ESP_OK;
}

struct TimerInit {
    SemaphoreHandle_t done;
    esp_err_t result;
};

// The interrupt is allocated on the CPU that registers the callbacks.
void timer_init_task(void* arg) {
    auto* ti = static_cast<TimerInit*>(arg);
    ti->result = [&]() -> esp_err_t {
        gptimer_config_t cfg = {};
        cfg.clk_src = GPTIMER_CLK_SRC_DEFAULT;
        cfg.direction = GPTIMER_COUNT_UP;
        cfg.resolution_hz = 10 * 1000 * 1000;  // 0.1 us
        cfg.intr_priority = 3;
        ESP_RETURN_ON_ERROR(gptimer_new_timer(&cfg, &g_timer), TAG, "timer");
        gptimer_event_callbacks_t cbs = {};
        cbs.on_alarm = on_column;
        ESP_RETURN_ON_ERROR(gptimer_register_event_callbacks(g_timer, &cbs, nullptr), TAG, "cb");
        gptimer_alarm_config_t alarm = {};
        alarm.alarm_count = CONFIG_IGPP_COLUMN_PERIOD_TENTHS_US;
        alarm.reload_count = 0;
        alarm.flags.auto_reload_on_alarm = true;
        ESP_RETURN_ON_ERROR(gptimer_set_alarm_action(g_timer, &alarm), TAG, "alarm");
        ESP_RETURN_ON_ERROR(gptimer_enable(g_timer), TAG, "enable");
        ESP_RETURN_ON_ERROR(gptimer_start(g_timer), TAG, "start");
        return ESP_OK;
    }();
    xSemaphoreGive(ti->done);
    vTaskDelete(nullptr);
}

// If the scan stops advancing, deselect the cathodes and reboot: a stuck
// column would otherwise stay lit with full duty.
void monitor_task(void*) {
    uint32_t last = g_scanner.frames();
    TickType_t last_change = xTaskGetTickCount();
    const TickType_t limit = pdMS_TO_TICKS(CONFIG_IGPP_STALL_TIMEOUT_MS);
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(5));
        const uint32_t now_frames = g_scanner.frames();
        const TickType_t now = xTaskGetTickCount();
        if (now_frames != last) {
            last = now_frames;
            last_change = now;
        } else if (now - last_change > limit) {
            emergency_off();
            ESP_EARLY_LOGE(TAG, "scan stalled for %d ms, rebooting", CONFIG_IGPP_STALL_TIMEOUT_MS);
            esp_restart();
        }
    }
}

}  // namespace

esp_err_t start() {
    configure_outputs();
    ESP_RETURN_ON_ERROR(configure_spi(), TAG, "spi");
    g_port.anode_send(kBlankColumn.data());
    g_port.latch_all();  // anodes dark, cathodes still deselected

    g_submit_mutex = xSemaphoreCreateMutex();

    TimerInit ti = {xSemaphoreCreateBinary(), ESP_FAIL};
    xTaskCreatePinnedToCore(timer_init_task, "scan_init", 3072, &ti, configMAX_PRIORITIES - 1,
                            nullptr, 1);
    xSemaphoreTake(ti.done, portMAX_DELAY);
    vSemaphoreDelete(ti.done);
    ESP_RETURN_ON_ERROR(ti.result, TAG, "scan timer");

    xTaskCreatePinnedToCore(monitor_task, "scan_mon", 3072, nullptr, configMAX_PRIORITIES - 2,
                            nullptr, 0);
    ESP_LOGI(TAG, "scan started: column %d.%d us, frame %.1f Hz",
             CONFIG_IGPP_COLUMN_PERIOD_TENTHS_US / 10, CONFIG_IGPP_COLUMN_PERIOD_TENTHS_US % 10,
             1e7 / (double(CONFIG_IGPP_COLUMN_PERIOD_TENTHS_US) * kWidth));
    return ESP_OK;
}

bool submit(const Frame& frame, TickType_t wait) {
    const TickType_t t0 = xTaskGetTickCount();
    if (xSemaphoreTake(g_submit_mutex, wait) != pdTRUE) {
        return false;
    }
    while (g_scanner.swap_pending()) {
        if (xTaskGetTickCount() - t0 >= wait) {
            xSemaphoreGive(g_submit_mutex);
            return false;
        }
        vTaskDelay(1);
    }
    std::memcpy(g_scanner.back().data(), frame.data(), kFrameBytes);
    g_scanner.publish();
    g_shown.fetch_add(1, std::memory_order_relaxed);
    xSemaphoreGive(g_submit_mutex);
    return true;
}

void blank() {
    static Frame dark;
    fill(dark, false);
    submit(dark, portMAX_DELAY);
}

void emergency_off() {
    g_stopped.store(true);
    // Let a column that is in flight on CPU1 finish, then deselect cathodes.
    esp_rom_delay_us(CONFIG_IGPP_COLUMN_PERIOD_TENTHS_US / 10 + 50);
    g_scanner.cathode_off();
}

uint32_t scan_frames() { return g_scanner.frames(); }
uint32_t shown_frames() { return g_shown.load(std::memory_order_relaxed); }

}  // namespace igpp::display
