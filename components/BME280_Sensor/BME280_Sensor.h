#ifndef BME280_SENSOR_H
#define BME280_SENSOR_H

/*
 * BME280 sensor interface for ESP32-S3.
 *
 * This module uses Espressif's BME280 component. That component uses the
 * Espressif I2C bus component internally; on ESP-IDF 5.3+ (including 6.1)
 * that component uses the newer I2C master driver.
 */

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "bme280.h"

/* The BME280 has two possible 7-bit I2C addresses, selected by the SDO pin. */
#define BME280_I2C_ADDR_PRIMARY    0x76
#define BME280_I2C_ADDR_SECONDARY  0x77

/*
 * All hardware and sensor setup values live in this structure.
 *
 * Edit the values in main.c and pass the structure to
 * bme280_sensor_init(). The I2C address is a 7-bit address (0x76 or 0x77),
 * not an address with a read/write bit appended.
 */
typedef struct
{
    i2c_port_t i2c_port;              // I2C controller number, normally I2C_NUM_0
    int sda_gpio;                     // ESP32-S3 GPIO connected to sensor SDA
    int scl_gpio;                     // ESP32-S3 GPIO connected to sensor SCL
    uint8_t i2c_address;              // BME280 address: 0x76 or 0x77
    uint32_t i2c_frequency_hz;        // I2C clock speed; 100000 = 100 kHz
    bool enable_internal_pullups;     // Internal pullups; external pullups are recommended

    /* Measurement settings passed to Espressif's BME280 driver. */
    bme280_sensor_mode mode;
    bme280_sensor_sampling temperature_sampling;
    bme280_sensor_sampling pressure_sampling;
    bme280_sensor_sampling humidity_sampling;
    bme280_sensor_filter filter;
    bme280_standby_duration standby;
} bme280_sensor_config_t;

/* Values that work as a starting point for a typical ESP32-S3 + BME280 setup. */
#define BME280_SENSOR_CONFIG_DEFAULT() {             \
    .i2c_port = I2C_NUM_0,                           \
    .sda_gpio = 10,                                  \
    .scl_gpio = 11,                                  \
    .i2c_address = BME280_I2C_ADDR_PRIMARY,           \
    .i2c_frequency_hz = 100000,                      \
    .enable_internal_pullups = true,                 \
    .mode = BME280_MODE_NORMAL,                      \
    .temperature_sampling = BME280_SAMPLING_X1,      \
    .pressure_sampling = BME280_SAMPLING_X1,         \
    .humidity_sampling = BME280_SAMPLING_X1,         \
    .filter = BME280_FILTER_OFF,                     \
    .standby = BME280_STANDBY_MS_1000                \
}

/* Clean application-level data type; avoids colliding with bme280.h's
 * internal calibration-data type, which is also named bme280_data_t. */
typedef struct
{
    float temperature_c;  // Degrees Celsius
    float humidity_pct;   // Relative humidity, percent
    float pressure_hpa;   // Pressure, hectopascals (hPa)
    bool is_valid;        // true only when all three readings succeeded
} bme280_reading_t;

/**
 * @brief Start the I2C bus and initialize the BME280 sensor.
 * @param config Pointer to the settings structure. Must remain valid only
 *               for this call; the values are copied into driver setup.
 * @return ESP_OK on success, otherwise an ESP-IDF error code.
 */
esp_err_t bme280_sensor_init(i2c_bus_handle_t shared_bus, const bme280_sensor_config_t *config);


/**
 * @brief Read temperature, humidity, and pressure from the sensor.
 * @param reading Output structure filled with the latest measurements.
 * @return ESP_OK when all readings succeed, otherwise an ESP-IDF error.
 */
esp_err_t bme280_sensor_read(bme280_reading_t *reading);

/**
 * @brief Release the sensor and I2C bus resources.
 * @return ESP_OK when resources are released.
 */
esp_err_t bme280_sensor_deinit(void);

#endif /* BME280_SENSOR_H */
