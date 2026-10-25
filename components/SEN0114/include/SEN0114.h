/**
 * @file SEN0114.h
 * @author Anthony Vu   
 * @brief Header file for the SEN0114 moisture sensor.
 * @version 1.0 
 * 
 * @details This header file provides the function declarations and necessary
 * configurations for interfacing with the SEN0114 moisture sensor using the
 * ESP32 ADC. It includes functions for initializing, deinitializing, and
 * reading moisture values from the sensor.
 */

#pragma once

#define MOISTURE_ADC_CHANNEL ADC_CHANNEL_1
 
/**
 * @brief Initializes the SEN0114 sensor.
 *
 * This function initializes the ADC unit and configures the channel for reading
 * moisture values from the SEN0114 sensor.
 */
void SEN0114_init();

/**
 * @brief Deinitializes the SEN0114 sensor.
 * 
 * This function deinitializes the ADC unit used for reading moisture values
 * from the SEN0114 sensor.
 */
void SEN0114_deinit();

/**
 * @brief Reads the moisture value from the SEN0114 sensor.
 *
 * This function reads the moisture value from the SEN0114 sensor using the
 * configured ADC channel and returns the raw ADC value.
 *
 * @return The raw ADC value representing the moisture level.
 */
int SEN0114_read();
