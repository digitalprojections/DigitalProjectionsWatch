idf_component_register(
    SRCS "storage_service.c"
    INCLUDE_DIRS "include"
    REQUIRES dp_watch_board fatfs esp_hw_support esp_timer
)
