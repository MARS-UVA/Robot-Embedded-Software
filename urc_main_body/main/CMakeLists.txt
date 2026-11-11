# The application uses the reusable local bme280_sensor component.
idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES BME280_Sensor
)
