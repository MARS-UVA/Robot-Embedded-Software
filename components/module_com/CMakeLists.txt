idf_component_register(SRCS "module_com.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_driver_twai esp_timer)
# TODO: check for required dependencies