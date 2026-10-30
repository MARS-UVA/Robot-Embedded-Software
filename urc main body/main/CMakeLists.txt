idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS ".")

idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES BME280_Sensor
)

