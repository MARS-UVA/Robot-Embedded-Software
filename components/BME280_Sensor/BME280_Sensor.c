/*
 * BME280.c
 *
 * Implementation of the BME280 sensor module.
 *
 * Responsibilities:
 *   1. Accept a pre-configured and active I2C master bus handle.
 *   2. Create and initialize the BME280 device using the underlying component.
 *   3. Safely manage and read environmental measurements.
 *   4. Cleanly release sensor handles when the application is finished.
 */

#include "BME280_Sensor.h"
#include "esp_log.h"
#include "i2c_bus.h"

static const char *TAG = "BME280";

/* These handles are private to this file. Other modules use the API functions
 * rather than accessing the driver handles directly. */
static i2c_bus_handle_t s_i2c_bus = NULL;
static bme280_handle_t s_sensor = NULL;
static bool s_initialized = false;
static bme280_sensor_mode s_mode = BME280_MODE_NORMAL;

esp_err_t bme280_sensor_init(i2c_bus_handle_t shared_bus, const bme280_sensor_config_t *config)
{
    if (shared_bus == NULL) {
        ESP_LOGE(TAG, "Provided I2C bus handle is NULL. Initialize the bus first in app_main!");
        return ESP_ERR_INVALID_ARG;
    }

    if (config == NULL) {
        ESP_LOGE(TAG, "Configuration pointer is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    if (s_initialized) {
        ESP_LOGW(TAG, "Sensor is already initialized");
        return ESP_OK;
    }

    /* Validate sensor specific configurations */
    if ((config->i2c_address != BME280_I2C_ADDR_PRIMARY &&
         config->i2c_address != BME280_I2C_ADDR_SECONDARY) ||
        (config->mode != BME280_MODE_NORMAL &&
         config->mode != BME280_MODE_FORCED)) {
        ESP_LOGE(TAG, "Invalid BME280 address or operational mode configured");
        return ESP_ERR_INVALID_ARG;
    }

    /* Assign the active, shared I2C bus context */
    s_i2c_bus = shared_bus;

    /* Create the BME280 device handle on the running shared I2C bus. */
    s_sensor = bme280_create(s_i2c_bus, config->i2c_address);
    if (s_sensor == NULL) {
        ESP_LOGE(TAG, "Could not create BME280 device handle on specified I2C bus");
        s_i2c_bus = NULL;
        return ESP_FAIL;
    }

    /* Check the chip ID, read calibration data, and perform standard initialization.
     * This relies on the I2C master bus being fully active. */
    esp_err_t err = bme280_default_init(s_sensor);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "BME280 default init failed. Verify wiring, address config, and pull-up resistors.");
        bme280_delete(&s_sensor);
        s_i2c_bus = NULL;
        return err;
    }

    /* Apply the measurement options supplied by the application profile. */
    err = bme280_set_sampling(
        s_sensor,
        config->mode,
        config->temperature_sampling,
        config->pressure_sampling,
        config->humidity_sampling,
        config->filter,
        config->standby
    );

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Could not apply BME280 sampling configuration");
        bme280_delete(&s_sensor);
        s_i2c_bus = NULL;
        return err;
    }

    s_mode = config->mode;
    s_initialized = true;
    ESP_LOGI(TAG, "BME280 device successfully initialized on shared bus at address 0x%02X", config->i2c_address);

    return ESP_OK;
}

esp_err_t bme280_sensor_read(bme280_reading_t *reading)
{
    if (reading == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    /* Start with safe default values in case a read transaction fails. */
    reading->temperature_c = 0.0f;
    reading->humidity_pct = 0.0f;
    reading->pressure_hpa = 0.0f;
    reading->is_valid = false;

    if (!s_initialized || s_sensor == NULL) {
        ESP_LOGE(TAG, "Read requested before sensor initialization was successful");
        return ESP_ERR_INVALID_STATE;
    }

    /* In forced mode, the sensor sleeps between measurements. Ask it to
     * execute a fresh conversion before fetching the hardware registers. */
    if (s_mode == BME280_MODE_FORCED) {
        esp_err_t measure_err = bme280_take_forced_measurement(s_sensor);
        if (measure_err != ESP_OK) {
            ESP_LOGW(TAG, "Forced measurement request failed on I2C transaction");
            return measure_err;
        }
    }

    /* Read environmental data quantities from the BME280 registers. */
    esp_err_t err = bme280_read_temperature(s_sensor, &reading->temperature_c);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Temperature register read transaction failed");
        return err;
    }

    err = bme280_read_humidity(s_sensor, &reading->humidity_pct);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Humidity register read transaction failed");
        return err;
    }

    /* Espressif's component automatically processes data into hPa. */
    err = bme280_read_pressure(s_sensor, &reading->pressure_hpa);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Pressure register read transaction failed");
        return err;
    }

    reading->is_valid = true;
    return ESP_OK;
}

esp_err_t bme280_sensor_deinit(void)
{
    esp_err_t result = ESP_OK;

    if (s_sensor != NULL) {
        result = bme280_delete(&s_sensor);
    }

    /* Cleared locally without deleting the parent I2C bus context, 
     * since the lifecycle of the shared bus is managed externally. */
    s_i2c_bus = NULL; 
    s_initialized = false;
    s_mode = BME280_MODE_NORMAL;
    
    ESP_LOGI(TAG, "BME280 device sensor handles released smoothly");
    return result;
}
