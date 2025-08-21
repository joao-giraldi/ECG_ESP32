file(GLOB SOURCES "*.c")

idf_component_register (
    SRCS ${SOURCES}
    PRIV_REQUIRES esp_adc ads111x i2cdev esp_idf_lib_helpers
    INCLUDE_DIRS ""
)