file(GLOB SOURCES "*.c")

idf_component_register (
    SRCS ${SOURCES}
<<<<<<< HEAD
    PRIV_REQUIRES ads111x i2cdev esp_idf_lib_helpers fatfs esp_wifi nvs_flash esp_netif esp_event esp_http_server esp_timer
=======
    PRIV_REQUIRES ads111x i2cdev esp_idf_lib_helpers fatfs esp_wifi nvs_flash esp_netif esp_event esp_http_server mdns
>>>>>>> 526197492bf1766faa37293da587f7a1f865e641
    INCLUDE_DIRS ""
)

littlefs_create_partition_image(graphics ../littlefs_image FLASH_IN_PROJECT)