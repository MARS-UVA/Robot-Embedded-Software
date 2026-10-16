idf_component_register(SRCS "can.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver esp_driver_gpio)

# Keep the existing TWAI API for this refactor. IDF 6 emits a deprecation
# preprocessor warning; show it without failing this component's build.
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error=cpp)
