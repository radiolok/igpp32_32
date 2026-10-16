#include "storage.hpp"

#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "igpp/board.hpp"
#include "sdmmc_cmd.h"
#include "wear_levelling.h"

namespace storage {

namespace {

constexpr const char* TAG = "storage";
bool g_sd = false;
bool g_flash = false;
wl_handle_t g_wl = WL_INVALID_HANDLE;

void mount_sd() {
    using namespace igpp::board;
    spi_bus_config_t bus = {};
    bus.mosi_io_num = kSdMosi;
    bus.miso_io_num = kSdMiso;
    bus.sclk_io_num = kSdSck;
    bus.quadwp_io_num = -1;
    bus.quadhd_io_num = -1;
    bus.max_transfer_sz = 4096;
    esp_err_t err = spi_bus_initialize(SPI3_HOST, &bus, SPI_DMA_CH_AUTO);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SPI3 init failed: %s", esp_err_to_name(err));
        return;
    }

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SPI3_HOST;
    host.max_freq_khz = 10000;  // breadboard wiring: stay conservative

    sdspi_device_config_t slot = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot.gpio_cs = kSdCs;
    slot.host_id = SPI3_HOST;

    esp_vfs_fat_sdmmc_mount_config_t cfg = {};
    cfg.format_if_mount_failed = false;
    cfg.max_files = 4;
    cfg.allocation_unit_size = 16 * 1024;

    sdmmc_card_t* card = nullptr;
    err = esp_vfs_fat_sdspi_mount(kSdRoot, &host, &slot, &cfg, &card);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "no SD card (%s)", esp_err_to_name(err));
        return;
    }
    g_sd = true;
    sdmmc_card_print_info(stdout, card);
}

void mount_flash() {
    esp_vfs_fat_mount_config_t cfg = {};
    cfg.format_if_mount_failed = true;
    cfg.max_files = 4;
    cfg.allocation_unit_size = CONFIG_WL_SECTOR_SIZE;
    const esp_err_t err = esp_vfs_fat_spiflash_mount_rw_wl(kFlashRoot, "storage", &cfg, &g_wl);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "flash FAT mount failed: %s", esp_err_to_name(err));
        return;
    }
    g_flash = true;
}

}  // namespace

void mount() {
    mount_sd();
    mount_flash();
    ESP_LOGI(TAG, "SD: %s, flash: %s", g_sd ? "yes" : "no", g_flash ? "yes" : "no");
}

bool sd_mounted() { return g_sd; }
bool flash_mounted() { return g_flash; }

const char* upload_root() {
    if (g_sd) return kSdRoot;
    if (g_flash) return kFlashRoot;
    return nullptr;
}

}  // namespace storage
