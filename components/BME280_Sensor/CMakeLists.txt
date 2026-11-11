# Build the BME280 wrapper as a reusable ESP-IDF component.
# i2c_bus is listed explicitly because BME280.h exposes its I2C port type
# through Espressif's BME280 driver headers.
idf_component_register(
    SRCS "BME280_Sensor.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES espressif__bme280 espressif__i2c_bus
)
