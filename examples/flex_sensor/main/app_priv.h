/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#pragma once
#include <stdint.h>
#include <stdbool.h>

#define DEFAULT_TEMPERATURE 25.0
#define REPORTING_PERIOD    5 /* Seconds */

// Look up the ESP programming guide to see which pin is ADC1_CH0
#define ADC_UNIT ADC_UNIT_1
#define ADC_PIN ADC_CHANNEL_0 // pin 1 for ESP32-S3

// Can ignore the following four lines
#define ADC_ATTEN ADC_ATTEN_DB_11
#define ADC_BITWIDTH ADC_BITWIDTH_DEFAULT
#define ADC_RAW_MAX (4095)
#define ADC_RAIN_RAW_MIN (1400) // recorded min = 1441 TODO=: To confirm these values
#define ADC_RAIN_RAW_MAX (3800) // recorded max = 3728
#define SENSOR_RANGE (100)

#define DEFAULT_SPEED       0 // slowest

// Can define your own threshold for bright and dark here
// you can then use these constants to decide what to do when certain thresholds are met
#define LDR_BRIGHT (80)
#define LDR_DARK (20)

extern esp_rmaker_device_t *temp_sensor_device;

void app_driver_init(void);
float app_get_current_temperature();
int get_sensor_reading(void);
static void app_sensor_update(TimerHandle_t handle);
int IRAM_ATTR app_driver_set_level(int level);
