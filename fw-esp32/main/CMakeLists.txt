idf_component_register(SRCS "main.cpp" "player.cpp" "storage.cpp" "usb_link.cpp"
                       INCLUDE_DIRS "."
                       REQUIRES igpp_core igpp_display driver fatfs sdmmc wear_levelling
                                esp_timer vfs log freertos)
