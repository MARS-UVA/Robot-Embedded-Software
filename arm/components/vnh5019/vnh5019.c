#include "vnh5019.h"
#include "esp_check.h"

static const char *TAG = "vnh5019";

struct vnh5019_t {
    gpio_num_t ina;
    gpio_num_t inb;
    gpio_num_t cs;
    gpio_num_t enadiaga;
    gpio_num_t enbdiagb;
    
    ledc_channel_t channel;
    ledc_mode_t speed_mode; /**< LEDC channel mode, LEDC_HIGH_SPEED_MODE is only available for the ESP32 */
    ledc_timer_t timer;
    ledc_timer_bit_t duty_resolution;
    uint32_t freq_hz;
};

esp_err_t vnh5019_init(vnh5019_handle_t *handle, const vnh5019_config_t *config)
{
    esp_err_t ret = ESP_OK;
    *handle = NULL;
    
    if (handle == NULL || config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    vnh5019_handle_t dev = (vnh5019_handle_t)malloc(sizeof(*dev));
    if (dev == NULL) {
        return ESP_ERR_NO_MEM;
    }

    ledc_channel_config_t ledc_config = {
        .gpio_num = config->pwm,
        .speed_mode = config->speed_mode,
        .channel = config->channel,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = config->timer,
        .duty = 0,
        .hpoint = 0
    };

    ESP_GOTO_ON_ERROR(ledc_channel_config(&ledc_config), fail, TAG, "Failed to configure LEDC channel %d", config->channel);

    ESP_GOTO_ON_ERROR(gpio_reset_pin(config->ina), fail, TAG, "Failed to reset GPIO pin %d", config->ina);
    ESP_GOTO_ON_ERROR(gpio_set_direction(config->ina, GPIO_MODE_OUTPUT), fail, TAG, "Failed to set GPIO direction for pin %d", config->ina);

    ESP_GOTO_ON_ERROR(gpio_reset_pin(config->inb), fail, TAG, "Failed to reset GPIO pin %d", config->inb);
    ESP_GOTO_ON_ERROR(gpio_set_direction(config->inb, GPIO_MODE_OUTPUT), fail, TAG, "Failed to set GPIO direction for pin %d", config->inb);
    
    ESP_GOTO_ON_ERROR(gpio_reset_pin(config->pwm), fail, TAG, "Failed to reset GPIO pin %d", config->pwm);
    ESP_GOTO_ON_ERROR(gpio_set_direction(config->pwm, GPIO_MODE_OUTPUT), fail, TAG, "Failed to set GPIO direction for pin %d", config->pwm);

    if (config->cs != GPIO_NUM_NC) {
        ESP_GOTO_ON_ERROR(gpio_reset_pin(config->cs), fail, TAG, "Failed to reset GPIO pin %d", config->cs);
        ESP_GOTO_ON_ERROR(gpio_set_direction(config->cs, GPIO_MODE_INPUT), fail, TAG, "Failed to set GPIO direction for pin %d", config->cs);
    }

    if (config->enadiaga != GPIO_NUM_NC) {
        ESP_GOTO_ON_ERROR(gpio_reset_pin(config->enadiaga), fail, TAG, "Failed to reset GPIO pin %d", config->enadiaga);
        ESP_GOTO_ON_ERROR(gpio_set_direction(config->enadiaga, GPIO_MODE_INPUT), fail, TAG, "Failed to set GPIO direction for pin %d", config->enadiaga);
    }

    if (config->enbdiagb != GPIO_NUM_NC) {
        ESP_GOTO_ON_ERROR(gpio_reset_pin(config->enbdiagb), fail, TAG, "Failed to reset GPIO pin %d", config->enbdiagb);
        ESP_GOTO_ON_ERROR(gpio_set_direction(config->enbdiagb, GPIO_MODE_INPUT), fail, TAG, "Failed to set GPIO direction for pin %d", config->enbdiagb);
    }

    dev->ina = config->ina;
    dev->inb = config->inb;
    dev->channel = config->channel;
    dev->speed_mode = config->speed_mode;
    dev->timer = config->timer;
    dev->duty_resolution = config->duty_resolution;
    dev->freq_hz = config->freq_hz;
    dev->cs = config->cs;
    dev->enadiaga = config->enadiaga;
    dev->enbdiagb = config->enbdiagb;

    *handle = dev;
    return ESP_OK;

    fail:
        free(dev);
        *handle = NULL;
        return ret;
}

esp_err_t vnh5019_deinit(vnh5019_handle_t handle)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // TODO: Make this deconfigure the LEDC channel using the deconfigure member of the ledc_channel_config_t struct. Need ESP-IDF 6.1 or higher
    ESP_RETURN_ON_ERROR(ledc_stop(handle->speed_mode, handle->channel, 0), TAG, "Failed to stop LEDC channel %d", handle->channel);

    ESP_RETURN_ON_ERROR(gpio_reset_pin(handle->ina), TAG, "Failed to reset GPIO pin %d", handle->ina);
    ESP_RETURN_ON_ERROR(gpio_reset_pin(handle->inb), TAG, "Failed to reset GPIO pin %d", handle->inb);
    
    ESP_RETURN_ON_ERROR(gpio_reset_pin(handle->cs), TAG, "Failed to reset GPIO pin %d", handle->cs);
    ESP_RETURN_ON_ERROR(gpio_reset_pin(handle->enadiaga), TAG, "Failed to reset GPIO pin %d", handle->enadiaga);
    ESP_RETURN_ON_ERROR(gpio_reset_pin(handle->enbdiagb), TAG, "Failed to reset GPIO pin %d", handle->enbdiagb);

    free(handle);
    return ESP_OK;
}

esp_err_t vnh5019_set_output(vnh5019_handle_t handle, float duty_cycle)
{
    if (handle == NULL || duty_cycle < -1.0f || duty_cycle > 1.0f) {
        return ESP_ERR_INVALID_ARG;
    }

    if (duty_cycle > 0.0f) {
        ESP_RETURN_ON_ERROR(gpio_set_level(handle->ina, 1), TAG, "Failed to set GPIO level for pin %d", handle->ina);
        ESP_RETURN_ON_ERROR(gpio_set_level(handle->inb, 0), TAG, "Failed to set GPIO level for pin %d", handle->inb);
    } else if (duty_cycle == 0.0f) {
        ESP_RETURN_ON_ERROR(gpio_set_level(handle->ina, 0), TAG, "Failed to set GPIO level for pin %d", handle->ina);
        ESP_RETURN_ON_ERROR(gpio_set_level(handle->inb, 0), TAG, "Failed to set GPIO level for pin %d", handle->inb);
    } else {
        ESP_RETURN_ON_ERROR(gpio_set_level(handle->ina, 0), TAG, "Failed to set GPIO level for pin %d", handle->ina);
        ESP_RETURN_ON_ERROR(gpio_set_level(handle->inb, 1), TAG, "Failed to set GPIO level for pin %d", handle->inb);
    }
    ESP_RETURN_ON_ERROR(ledc_set_duty_and_update(handle->speed_mode, handle->channel, (uint32_t)(duty_cycle * ((1 << handle->duty_resolution) - 1)), 0), TAG, "Failed to set duty and update for channel %d", handle->channel);
    return ESP_OK;
}

// TODO: Implement current sensing and fault detection using the CS, ENA_DIAGA, and ENB_DIAGB pins.
