#include "BME280_Sensor.h"
#include "esp_log.h"
#include "bme280.h"         // Espressif component library header

static const char *TAG = "BME280_Driver";
static bme280_handle_t sensor_handle = NULL;
static bool sensor_ready = false;

bool network_bme280_init(int sda_pin, int scl_pin, uint8_t i2c_address) {
    // 1. Setup the I2C Master Bus Configuration
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = sda_pin,
        .scl_io_num = scl_pin,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    
    i2c_master_bus_handle_t bus_handle;
    if (i2c_new_master_bus(&bus_config, &bus_handle) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2C Master Bus Context");
        return false;
    }

    // 2. Instantiate Espressif's BME280 Device Handle configuration
    bme280_config_t sensor_config = {
        .i2c_address = i2c_address,
    };

    if (bme280_create(bus_handle, &sensor_config, &sensor_handle) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to link Espressif BME280 Instance handle");
        sensor_ready = false;
        return false;
    }

    ESP_LOGI(TAG, "BME280 Sensor Connected and Operational");
    sensor_ready = true;
    return true;
}

void network_bme280_get_readings(bme280_data_t *out_data) {
    if (!out_data) return;

    if (!sensor_ready || sensor_handle == NULL) {
        out_data->is_valid = false;
        return;
    }

    // 3. Directly call the official Espressif retrieval macros
    float temp = 0.0f, hum = 0.0f, pres = 0.0f;
    
    esp_err_t ret_t = bme280_read_temperature(sensor_handle, &temp);
    esp_err_t ret_h = bme280_read_humidity(sensor_handle, &hum);
    esp_err_t ret_p = bme280_read_pressure(sensor_handle, &pres);

    if (ret_t == ESP_OK && ret_h == ESP_OK && ret_p == ESP_OK) {
        out_data->temperature = temp;
        out_data->humidity = hum;
        out_data->pressure = pres / 100.0f; // Convert Pa to hPa
        out_data->is_valid = true;
    } else {
        ESP_LOGE(TAG, "Failed to poll register bus telemetry from hardware target");
        out_data->is_valid = false;
    }
}
