/*  Temperature Sensor demo implementation using RGB LED and timer

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <esp_log.h>
#include <sdkconfig.h>
#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_types.h> 
#include <esp_rmaker_standard_params.h> 

#include <app_reset.h>
#include <ws2812_led.h>
#include "app_priv.h"

#include <driver/gpio.h>
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

/* This is the button that is used for toggling the power */
#define BUTTON_GPIO          CONFIG_EXAMPLE_BOARD_BUTTON_GPIO
#define BUTTON_ACTIVE_LEVEL  0
/* This is the GPIO on which the power will be set */
#define OUTPUT_GPIO    19

static TimerHandle_t sensor_timer;

#define DEFAULT_SATURATION  100
#define DEFAULT_BRIGHTNESS  50

#define WIFI_RESET_BUTTON_TIMEOUT       3
#define FACTORY_RESET_BUTTON_TIMEOUT    10

static uint16_t g_hue;
static uint16_t g_saturation = DEFAULT_SATURATION;
static uint16_t g_value = DEFAULT_BRIGHTNESS;
static float g_temperature; // TODO=: Replace name

static adc_oneshot_unit_handle_t adc_handle;
static adc_cali_handle_t adc_cali_handle;

static const char *TAG = "app_driver";

int err_timer;

int IRAM_ATTR app_driver_set_level(int level)
{
    switch (level) {
        case 0:
            ESP_LOGI(TAG, "level 0, break");
            err_timer = xTimerDelete(sensor_timer, 1); // xBlockTime set to 1 tick to prevent alarm_timer not deleted before next line, NULL only throws warning for casting type
            if (err_timer != 1) {
                ESP_LOGE(TAG, "Could not xTimerDelete, err: %i", err_timer);
                vTaskDelay(5000/portTICK_PERIOD_MS);
            }
            g_temperature = DEFAULT_TEMPERATURE;
            sensor_timer = xTimerCreate("app_sensor_update_tm", (REPORTING_PERIOD * 1000) / portTICK_PERIOD_MS,
                                    pdTRUE, NULL, app_sensor_update);
            if (sensor_timer) {
                xTimerStart(sensor_timer, 0);
                g_hue = (100 - g_temperature) * 2;
                ws2812_led_set_hsv(g_hue, g_saturation, g_value);
            }
            break;
        case 1:
            ESP_LOGI(TAG, "level 1");
            err_timer = xTimerDelete(sensor_timer, 1); // xBlockTime set to 1 tick to prevent alarm_timer not deleted before next line, NULL only throws warning for casting type
            if (err_timer != 1) {
                ESP_LOGE(TAG, "Could not xTimerDelete, err: %i", err_timer);
                vTaskDelay(5000/portTICK_PERIOD_MS);
            }
            g_temperature = DEFAULT_TEMPERATURE;
            sensor_timer = xTimerCreate("app_sensor_update_tm", (4 * 1000) / portTICK_PERIOD_MS,
                                    pdTRUE, NULL, app_sensor_update);
            if (sensor_timer) {
                xTimerStart(sensor_timer, 0);
                g_hue = (100 - g_temperature) * 2;
                ws2812_led_set_hsv(g_hue, g_saturation, g_value);
            }
            break;
        case 2:
            ESP_LOGI(TAG, "level 2");
            err_timer = xTimerDelete(sensor_timer, 1); // xBlockTime set to 1 tick to prevent alarm_timer not deleted before next line, NULL only throws warning for casting type
            if (err_timer != 1) {
                ESP_LOGE(TAG, "Could not xTimerDelete, err: %i", err_timer);
                vTaskDelay(5000/portTICK_PERIOD_MS);
            }
            g_temperature = DEFAULT_TEMPERATURE;
            sensor_timer = xTimerCreate("app_sensor_update_tm", (1 * 1000) / portTICK_PERIOD_MS,
                                    pdTRUE, NULL, app_sensor_update);
            if (sensor_timer) {
                xTimerStart(sensor_timer, 0);
                g_hue = (100 - g_temperature) * 2;
                ws2812_led_set_hsv(g_hue, g_saturation, g_value);
            }
            break;
        default:
            ESP_LOGI(TAG, "level default, no break");
    }
    return ESP_OK;
}

int map_range(int x, int in_min, int in_max, int out_min, int out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

int get_sensor_reading(void)
{
    // Read values
    int raw_value = -1;
    adc_oneshot_read(adc_handle, ADC_PIN, &raw_value);
    int sensor_val = map_range(raw_value, ADC_RAIN_RAW_MIN, ADC_RAIN_RAW_MAX, 0, SENSOR_RANGE); // can't invert out_min and out_max by swapping them

    int inverted_sensor_val = 100 - sensor_val;

    // Comment out the line below to silence it
    ESP_LOGI(TAG, "Raw ADC value: %d, mapped sensor value: %d, inverted sensor value: %d", raw_value, sensor_val, inverted_sensor_val);

    return inverted_sensor_val;
}

static void app_sensor_update(TimerHandle_t handle)
{
    g_temperature = DEFAULT_TEMPERATURE;
    g_temperature = get_sensor_reading();
    ESP_LOGI(TAG, "Sensor reading: %f", g_temperature);
    // for cycling at 0.5 intervals
    // static float delta = 0.5;
    // g_temperature += delta;
    // if (g_temperature > 99) {
    //     delta = -0.5;
    // } else if (g_temperature < 1) {
    //     delta = 0.5;
    // }

    // TODO=: Remove this section when not needed
    g_hue = (100 - g_temperature) * 2;
    ESP_LOGI(TAG, "g_hue: %d", g_hue);
    ws2812_led_set_hsv(g_hue, g_saturation, g_value);
    esp_rmaker_param_update_and_report(
            esp_rmaker_device_get_param_by_type(temp_sensor_device, ESP_RMAKER_PARAM_TEMPERATURE),
            esp_rmaker_float(g_temperature));
}

float app_get_current_temperature() // TODO=: Rename
{
    return g_temperature;
}

esp_err_t app_sensor_init(void)
{
    // POLISH=: Can remove this part when not using inbuilt LED
    esp_err_t err = ws2812_led_init();
    if (err != ESP_OK) {
        return err;
    }

    // ADC for analog read
    adc_oneshot_unit_init_cfg_t adc_config = {
        .unit_id = ADC_UNIT,
    };

    adc_oneshot_new_unit(&adc_config, &adc_handle);

    adc_oneshot_chan_cfg_t channel_config = {
        .bitwidth = ADC_BITWIDTH,
        .atten = ADC_ATTEN,
    };

    adc_oneshot_config_channel(adc_handle, ADC_PIN, &channel_config);

    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT,
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH,
    };

    adc_cali_create_scheme_curve_fitting(&cali_config, &adc_cali_handle);

    g_temperature = DEFAULT_TEMPERATURE;
    sensor_timer = xTimerCreate("app_sensor_update_tm", (REPORTING_PERIOD * 1000) / portTICK_PERIOD_MS,
                            pdTRUE, NULL, app_sensor_update);
    if (sensor_timer) {
        xTimerStart(sensor_timer, 0);
        g_hue = (100 - g_temperature) * 2;
        ws2812_led_set_hsv(g_hue, g_saturation, g_value);
        return ESP_OK;
    }
    return ESP_FAIL;
}

esp_err_t app_gpio_init(void)
{
    /* Configure power */
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = 1,
    };

    uint64_t pin_mask = (((uint64_t)1 << OUTPUT_GPIO ));
    io_conf.pin_bit_mask = pin_mask;
    /* Configure the GPIO */
    gpio_config(&io_conf);
    gpio_set_level(OUTPUT_GPIO, false);
    return ESP_OK;
}

void app_driver_init()
{
    app_sensor_init();
    app_gpio_init();
    app_reset_button_register(app_reset_button_create(BUTTON_GPIO, BUTTON_ACTIVE_LEVEL),
                WIFI_RESET_BUTTON_TIMEOUT, FACTORY_RESET_BUTTON_TIMEOUT);
}
