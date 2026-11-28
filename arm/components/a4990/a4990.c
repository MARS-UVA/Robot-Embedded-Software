#include "a4990.h"
#include "esp_err.h"
#include "esp_check.h"

static const char *TAG = "a4990";

struct a4990_t {
    ledc_channel_t in1;
    ledc_channel_t in2;
    ledc_channel_t in3;
    ledc_channel_t in4;

    gpio_num_t inh; 
    gpio_num_t ef1;
    gpio_num_t ef2;

    ledc_mode_t speed_mode;
    ledc_timer_t timer;
    ledc_timer_bit_t duty_resolution;
    uint32_t freq_hz;
};

esp_err_t a4990_init(a4990_handle_t *handle, const a4990_config_t *config)
{
    esp_err_t ret = ESP_OK;
    *handle = NULL;

    if (handle == NULL || config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    a4990_handle_t dev = calloc(1, sizeof(*dev));
    if (dev == NULL) {
        return ESP_ERR_NO_MEM;
    }

    ledc_channel_config_t chan_config[4] = {
        {
            .gpio_num = config->in1.gpio,
            .speed_mode = config->speed_mode,
            .channel = config->in1.channel,
            .intr_type = LEDC_INTR_DISABLE,
            .timer_sel = config->timer,
            .duty = 0,
            .hpoint = 0
        },
        {
            .gpio_num = config->in2.gpio,
            .speed_mode = config->speed_mode,
            .channel = config->in2.channel,
            .intr_type = LEDC_INTR_DISABLE,
            .timer_sel = config->timer,
            .duty = 0,
            .hpoint = 0
        },
        {
            .gpio_num = config->in3.gpio,
            .speed_mode = config->speed_mode,
            .channel = config->in3.channel,
            .intr_type = LEDC_INTR_DISABLE,
            .timer_sel = config->timer,
            .duty = 0,
            .hpoint = 0
        },
        {
            .gpio_num = config->in4.gpio,
            .speed_mode = config->speed_mode,
            .channel = config->in4.channel,
            .intr_type = LEDC_INTR_DISABLE,
            .timer_sel = config->timer,
            .duty = 0,
            .hpoint = 0
        }
    };

    for (int i = 0; i < 4; i++) {
        ESP_GOTO_ON_ERROR(ledc_channel_config(&chan_config[i]), fail, TAG, "Failed to configure LEDC channel %d", i);
    }

    ESP_GOTO_ON_ERROR(gpio_reset_pin(config->inh), fail, TAG, "Failed to reset GPIO pin %d", config->inh);
    ESP_GOTO_ON_ERROR(gpio_set_direction(config->inh, GPIO_MODE_OUTPUT), fail, TAG, "Failed to set GPIO direction for pin %d", config->inh);
    ESP_GOTO_ON_ERROR(gpio_set_level(config->inh, 0), fail, TAG, "Failed to set GPIO level for pin %d", config->inh);

    ESP_GOTO_ON_ERROR(gpio_reset_pin(config->ef1), fail, TAG, "Failed to reset GPIO pin %d", config->ef1);
    ESP_GOTO_ON_ERROR(gpio_set_direction(config->ef1, GPIO_MODE_INPUT), fail, TAG, "Failed to set GPIO direction for pin %d", config->ef1);

    ESP_GOTO_ON_ERROR(gpio_reset_pin(config->ef2), fail, TAG, "Failed to reset GPIO pin %d", config->ef2);
    ESP_GOTO_ON_ERROR(gpio_set_direction(config->ef2, GPIO_MODE_INPUT), fail, TAG, "Failed to set GPIO direction for pin %d", config->ef2);

    dev->in1 = config->in1.channel;
    dev->in2 = config->in2.channel;
    dev->in3 = config->in3.channel;
    dev->in4 = config->in4.channel;

    dev->inh = config->inh;
    dev->ef1 = config->ef1;
    dev->ef2 = config->ef2;

    dev->speed_mode = config->speed_mode;
    dev->timer = config->timer;
    dev->duty_resolution = config->duty_resolution;
    dev->freq_hz = config->freq_hz;

    *handle = dev;
    return ret;

    fail:
        free(dev);
        *handle = NULL;
        return ret;

}

esp_err_t a4990_deinit(a4990_handle_t handle)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // TODO: Make this deconfigure the LEDC channels using the deconfigure member of the ledc_channel_config_t struct. Need ESP-IDF 6.1 or higher
    ESP_RETURN_ON_ERROR(ledc_stop(handle->speed_mode, handle->in1, 0), TAG, "Failed to stop LEDC channel %d", handle->in1);
    ESP_RETURN_ON_ERROR(ledc_stop(handle->speed_mode, handle->in2, 0), TAG, "Failed to stop LEDC channel %d", handle->in2);
    ESP_RETURN_ON_ERROR(ledc_stop(handle->speed_mode, handle->in3, 0), TAG, "Failed to stop LEDC channel %d", handle->in3);
    ESP_RETURN_ON_ERROR(ledc_stop(handle->speed_mode, handle->in4, 0), TAG, "Failed to stop LEDC channel %d", handle->in4);

    ESP_RETURN_ON_ERROR(gpio_reset_pin(handle->inh), TAG, "Failed to reset GPIO pin %d", handle->inh);
    ESP_RETURN_ON_ERROR(gpio_reset_pin(handle->ef1), TAG, "Failed to reset GPIO pin %d", handle->ef1);
    ESP_RETURN_ON_ERROR(gpio_reset_pin(handle->ef2), TAG, "Failed to reset GPIO pin %d", handle->ef2);

    free(handle);
    return ESP_OK;
}

esp_err_t a4990_enable(a4990_handle_t handle)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    return gpio_set_level(handle->inh, 1);
}

esp_err_t a4990_disable(a4990_handle_t handle)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    return gpio_set_level(handle->inh, 0);
}

esp_err_t a4990_set_output(a4990_handle_t handle, a4990_phase_t phase, float duty_cycle)
{
    if (handle == NULL || (phase != A4990_PHASE_A && phase != A4990_PHASE_B) || duty_cycle < -1.0f || duty_cycle > 1.0f) {
        return ESP_ERR_INVALID_ARG;
    }

    ledc_channel_t control_channel = (phase == A4990_PHASE_A) ? handle->in1 : handle->in3;
    ledc_channel_t inverted_control_channel = (phase == A4990_PHASE_A) ? handle->in2 : handle->in4;

    if (duty_cycle >= 0) {
        uint32_t duty = (uint32_t)(duty_cycle * ((1 << handle->duty_resolution) - 1));
        ESP_RETURN_ON_ERROR(ledc_set_duty_and_update(handle->speed_mode, control_channel, duty, 0), TAG, "Failed to set duty and update for channel %d", control_channel);
        ESP_RETURN_ON_ERROR(ledc_set_duty_and_update(handle->speed_mode, inverted_control_channel, 1, 0), TAG, "Failed to set duty and update for channel %d", inverted_control_channel);
        
    } else {
        uint32_t duty = (uint32_t)((1.0f - duty_cycle) * ((1 << handle->duty_resolution) - 1));
        ESP_RETURN_ON_ERROR(ledc_set_duty_and_update(handle->speed_mode, inverted_control_channel, duty, 0), TAG, "Failed to set duty and update for channel %d", inverted_control_channel);
        ESP_RETURN_ON_ERROR(ledc_set_duty_and_update(handle->speed_mode, control_channel, 0, 0), TAG, "Failed to set duty and update for channel %d", control_channel);
    }

    return ESP_OK;
}
