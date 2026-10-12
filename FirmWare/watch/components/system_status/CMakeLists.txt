idf_component_register(
    SRCS "system_status.cpp"
    INCLUDE_DIRS "include"
    REQUIRES
        brookesia_core
        dp_watch_board
        esp_driver_i2c
        esp_event
        esp_netif
        esp_wifi
        dp_time
        nvs_flash
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-missing-field-initializers)
