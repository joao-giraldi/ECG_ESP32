file(GLOB SOURCES "*.c")

idf_component_register (
    SRCS ${SOURCES}
    PRIV_REQUIRES spi_flash driver esp_adc
    INCLUDE_DIRS ""
)