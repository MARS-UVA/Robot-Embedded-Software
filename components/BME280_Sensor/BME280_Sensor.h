#ifndef BME280_SENSOR_H
#define BME280_SENSOR_H

#include <stdbool.h>
#include <stdint.h>

// Clean structural telemetry layout matching your driver specs
typedef struct {
    float temperature; // In Celsius
    float humidity;    // Relative Humidity %
    float pressure;    // In hPa
    bool is_valid;     // Operational status flag
} bme280_data_t;

/**
 * @brief Initializes the sensor utilizing Espressif's driver.
 * @return true if communication successfully opens, false on hardware fault.
 */
bool network_bme280_init(int sda_pin, int scl_pin, uint8_t i2c_address);

/**
 * @brief Queries the sensor and fills the passed data struct via pointer.
 * @param out_data Pointer to the destination telemetry struct.
 */
void network_bme280_get_readings(bme280_data_t *out_data);

#endif // BME280_SENSOR_H
