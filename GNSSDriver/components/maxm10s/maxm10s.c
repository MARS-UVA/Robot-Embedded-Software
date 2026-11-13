#include <stdio.h>
#include "maxm10s.h"
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
#include "esp_types.h"
#include "esp_check.h"

#define I2C_ADD 0x42
#define NUM_BYTES_REGISTER 0xFD
#define DATA_REGISTER 0xFF

static const char *TAG = "maxm10";

static i2c_master_bus_handle_t i2c_bus;
static i2c_master_dev_handle_t i2c_dev;

void printMax10sData(maxm10s_data_t GGA_data){

	printf("UTC: %d:%d:%.0f\n",GGA_data.utc_hour, GGA_data.utc_minute, GGA_data.utc_second);
	printf("Latitude: %f  Longitude: %f  Altitude: %.2f m\n",GGA_data.latitude, GGA_data.longitude, GGA_data.altitude);
	printf("Fix: %d  N of satellites: %d\n",GGA_data.fix_quality, GGA_data.num_satellites);
	printf("Horizontal dilution: %.2f  Height geoid: %.2f %c\n\n", GGA_data.horizontal_dilution, GGA_data.height_geoid, GGA_data.height_geoid_unit);

}

void parseNMEA_GGA (const char *nmeaGgaString, maxm10s_data_t *data){
    
    sscanf(nmeaGgaString,"$GNGGA,%2d%2d%f,%f,%c,%f,%c,%d,%d,%f,%f,%c,%f,%c",
           &data->utc_hour, &data->utc_minute, &data->utc_second,
           &data->latitude, &data->lat_N_S,
           &data->longitude, &data->lon_E_W,
           &data->fix_quality, &data->num_satellites, &data->horizontal_dilution,
           &data->altitude, &data->alt_unit, &data->height_geoid,
           &data->height_geoid_unit);
    
    //Adjust the latitude and longitude to decimal representation
    data->latitude = (int)(data->latitude / 100) +fmod(data->latitude, 100) /60.0;
    if(data->lat_N_S == 'S') data->latitude = -data->latitude;
    
    data->longitude = (int)(data->longitude / 100) +fmod(data->longitude, 100) /60.0;
    if(data->lon_E_W== 'W') data->longitude = -data->longitude;

}


char calculateChecksum(const char* inString) {
    char checksum = 0;
    //Discard the first character '$' - Counts up to the character '*'
    for (const char* p = inString+ 1; *p != '*' && *p != '\0'; p++) {
        checksum ^= *p;   //NMEA use XOR for checksum calculation
    }
    return checksum;
}

// Function to validate the checksum
bool validateChecksum(const char* nmeaString) {
    // Find the position of the '*' character
    const char* checksumPos= strchr(nmeaString, '*');
    if (checksumPos == NULL || checksumPos- nmeaString< 3) {
        return false;  // No '*' found or invalid string length
    }

    // Calculate the checksum
    char calculatedChecksum= calculateChecksum(nmeaString);

    // Extract the checksum from the string
    unsigned int receivedChecksum;
    sscanf(checksumPos + 1, "%2x", &receivedChecksum);

    // Compare the calculated checksum with the received checksum
    return calculatedChecksum == receivedChecksum;
}


esp_err_t maxm10s_init(maxm10s_config_t *config, maxm10s_dev_handle_t *dev_handle)
{
    i2c_master_bus_config_t bus_config = {
        .i2c_port = -1,
        .sda_io_num = config->sda_pin,
        .scl_io_num = config->scl_pin,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    esp_err_t ret = i2c_new_master_bus(&bus_config, &i2c_bus);

    ESP_RETURN_ON_ERROR(ret, TAG, "i2c bus initialization failed");

    dev_handle->sda_pin = config->sda_pin;
    dev_handle->scl_pin = config->scl_pin;
    dev_handle->i2c_bus_handle = &i2c_bus;

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = I2C_ADD,
        .scl_speed_hz = 400000,
    };


    ret = i2c_master_bus_add_device(*dev_handle->i2c_bus_handle, &dev_config, &i2c_dev);

    ESP_RETURN_ON_ERROR(ret, TAG, "i2c device intialization failed");

    dev_handle->i2c_dev_handle = &i2c_dev;

    return ret;

}

esp_err_t maxm10s_read(maxm10s_dev_handle_t *dev_handle, maxm10s_data_t *data) {
    uint8_t rx_data[16];
	char gps_string_in[250]; 
	int string_idx = 0;
    uint8_t curReg = NUM_BYTES_REGISTER;
    esp_err_t ret = i2c_master_transmit_receive(*dev_handle->i2c_dev_handle, &curReg, 1, rx_data, 2, 1000);
    ESP_RETURN_ON_ERROR(ret, TAG, "i2c transmit failed");
    ESP_LOGI(TAG, "ret=%s count bytes=%02X %02X", esp_err_to_name(ret), rx_data[0], rx_data[1]);
    uint32_t num_to_read = (uint32_t)rx_data[1] | ((uint32_t)rx_data[0] << 8);
    if(num_to_read == 0) {
        ESP_LOGI(TAG, "No data");
    }
    while(num_to_read > 0){
            size_t n;
            if (num_to_read > sizeof(rx_data)) {
                n = sizeof(rx_data);
            } else {
                n = num_to_read;
            }
            ret = i2c_master_receive(*(dev_handle->i2c_dev_handle), rx_data, n, -1);
            ESP_RETURN_ON_ERROR(ret, TAG, "i2c read failed");
            for (size_t i = 0; i < n; i++) {
                uint8_t cur = rx_data[i];
                if (cur == 0xFF) {
                    continue;
                }      

                if (cur == '\n') {
                    gps_string_in[string_idx] = '\0';
                    if (strstr(gps_string_in, "GNGGA") != NULL && validateChecksum(gps_string_in)) {
                        parseNMEA_GGA(gps_string_in, data);
                    }
                    string_idx = 0;
                } else if (cur != '\r') {
                    if (string_idx < (int)sizeof(gps_string_in) - 1) gps_string_in[string_idx++] = (char)cur;
                    else string_idx = 0;     
                }
            }
            num_to_read -= n;
    }
    return ret;
}