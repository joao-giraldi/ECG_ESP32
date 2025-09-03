file(GLOB SOURCES "*.c")

idf_component_register (
    SRCS ${SOURCES}
    PRIV_REQUIRES ads111x i2cdev esp_idf_lib_helpers fatfs esp_wifi nvs_flash esp_netif esp_event esp_http_server
    INCLUDE_DIRS ""
)

littlefs_create_partition_image(graphics ../littlefs_image FLASH_IN_PROJECT)