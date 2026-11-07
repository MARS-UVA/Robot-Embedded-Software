#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

typedef struct {
    gpio_num_t sda_pin;
    gpio_num_t scl_pin;
} maxm10s_config_t;

typedef struct {
    gpio_num_t sda_pin;
    gpio_num_t scl_pin;
    i2c_master_bus_handle_t *i2c_bus_handle;
    i2c_master_dev_handle_t *i2c_dev_handle;
} maxm10s_dev_handle_t;

typedef struct {
    int utc_hour;
    int utc_minute;
    float utc_second;
    float latitude;
    char lat_N_S;
    float longitude;
    char lon_E_W;
    int fix_quality;
    int num_satellites;
    float horizontal_dilution;
    float altitude;
    char alt_unit;
    float height_geoid;
    char height_geoid_unit;
} maxm10s_data_t;


esp_err_t maxm10s_init(maxm10s_config_t *config, maxm10s_dev_handle_t *dev_handle);

esp_err_t maxm10s_read(maxm10s_dev_handle_t *dev_handle, maxm10s_data_t *data);
