idf_component_register(SRCS "src/display.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES igpp_core driver hal soc esp_timer log freertos)
