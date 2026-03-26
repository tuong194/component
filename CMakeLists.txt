file(GLOB SRCS "src/*.c")
idf_component_register(SRCS ${SRCS}
                       INCLUDE_DIRS "inc"
                       PRIV_REQUIRES freertos log esp_hw_support bt nvs_flash
                       REQUIRES driver esp_rom esp_event )




