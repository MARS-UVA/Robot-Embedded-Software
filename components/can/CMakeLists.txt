idf_component_register(SRCS "can.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_hal_gpio
                    PRIV_REQUIRES esp_driver_twai)
