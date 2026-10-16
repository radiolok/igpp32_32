// ГИП10000 на стероидах — ESP32-S3 firmware.
//
// CPU1: column scan (IRAM interrupt, components/igpp_display).
// CPU0: USB link, player, storage, button, stall monitor.

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "igpp/board.hpp"
#include "igpp/display.hpp"
#include "player.hpp"
#include "storage.hpp"
#include "usb_link.hpp"

namespace {

constexpr const char* TAG = "main";

void button_task(void*) {
    bool was_down = false;
    TickType_t down_since = 0;
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(10));
        const bool down = gpio_get_level(igpp::board::kButton) == 0;
        const TickType_t now = xTaskGetTickCount();
        if (down && !was_down) {
            down_since = now;
        } else if (!down && was_down && now - down_since > pdMS_TO_TICKS(30)) {
            player::next();
        }
        was_down = down;
    }
}

}  // namespace

extern "C" void app_main() {
    ESP_ERROR_CHECK(igpp::display::start());

    gpio_config_t btn = {};
    btn.pin_bit_mask = 1ull << igpp::board::kButton;
    btn.mode = GPIO_MODE_INPUT;
    btn.pull_up_en = GPIO_PULLUP_ENABLE;
    ESP_ERROR_CHECK(gpio_config(&btn));
    vTaskDelay(pdMS_TO_TICKS(5));
    const bool test_mode = gpio_get_level(igpp::board::kButton) == 0;
    if (test_mode) {
        ESP_LOGW(TAG, "BOOT held at start: test mode, built-in patterns only");
    }

    storage::mount();
    player::start(test_mode);
    usb_link::start();
    xTaskCreatePinnedToCore(button_task, "button", 2048, nullptr, 4, nullptr, 0);

    uint32_t last_scan = igpp::display::scan_frames();
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        const uint32_t scan = igpp::display::scan_frames();
        ESP_LOGI(TAG, "scan %.1f Hz, shown %u, usb rx %u (dropped %u), item %u/%u, mode %u",
                 (scan - last_scan) / 10.0, unsigned(igpp::display::shown_frames()),
                 unsigned(usb_link::rx_frames()), unsigned(usb_link::rx_dropped()),
                 unsigned(player::index()), unsigned(player::size()), unsigned(player::mode()));
        last_scan = scan;
    }
}
