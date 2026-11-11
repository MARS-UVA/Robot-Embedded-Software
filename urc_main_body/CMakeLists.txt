
cmake_minimum_required(VERSION 3.16)

# Locate the shared components directory.
set(EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/../components"
)

# Print the component search directory.
message(STATUS "Custom component directory: ${EXTRA_COMPONENT_DIRS}")

# Check which component CMakeLists files actually exist.
file(GLOB COMPONENT_CMAKE_FILES
    "${EXTRA_COMPONENT_DIRS}/*/CMakeLists.txt"
)

message(STATUS "Discovered component CMakeLists files:")

foreach(COMPONENT_FILE ${COMPONENT_CMAKE_FILES})
    message(STATUS "  ${COMPONENT_FILE}")
endforeach()

# Confirm the BME280 component exists.
if(NOT EXISTS "${EXTRA_COMPONENT_DIRS}/bme280_sensor/CMakeLists.txt")
    message(FATAL_ERROR "BME280 component CMakeLists.txt was not found!")
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(urc_main_body)
