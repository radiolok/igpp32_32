// Content storage: microSD on SPI3 (/sd) and a FAT partition in flash (/flash).
#pragma once

#include "esp_err.h"

namespace storage {

constexpr const char* kSdRoot = "/sd";
constexpr const char* kFlashRoot = "/flash";

// Mount whatever is available. Never fails hard: missing SD is normal.
void mount();

bool sd_mounted();
bool flash_mounted();

// Where uploads go: SD if present, otherwise flash; nullptr if neither.
const char* upload_root();

}  // namespace storage
