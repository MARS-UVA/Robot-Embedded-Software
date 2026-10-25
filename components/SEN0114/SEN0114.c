/**
 * @file SEN0114.c
 * @author Anthony Vu
 * @brief Implementation for the SEN0114 moisture sensor.
 * @version 1.0
 * 
 * @details This source file provides the implementation of functions for
 * interfacing with the SEN0114 moisture sensor using the ESP32 ADC. It includes
 * functions for initializing, deinitializing, and reading moisture values from
 * the sensor.
 */

#include "SEN0114.h"
#include "esp_err.h"
#include "esp_adc/adc_oneshot.h"

adc_oneshot_unit_handle_t adc_handle;

adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };

void SEN0114_init() {
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    adc_oneshot_chan_cfg_t channel_config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };

    ESP_ERROR_CHECK(adc_oneshot_config_channel(
        adc_handle,
        MOISTURE_ADC_CHANNEL,
        &channel_config
    ));
}

void SEN0114_deinit() {
    ESP_ERROR_CHECK(adc_oneshot_del_unit(adc_handle));
}

int SEN0114_read() {
    int adc_value;
    ESP_ERROR_CHECK(adc_oneshot_read(adc_handle,
                                     MOISTURE_ADC_CHANNEL,
                                     &adc_value));
    return adc_value;
}
