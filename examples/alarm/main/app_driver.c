/* Switch demo implementation using button and RGB LED
   
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <esp_log.h>
#include <sdkconfig.h>

#include <iot_button.h>
#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_params.h> 

#include <app_reset.h>
#include <ws2812_led.h>
#include "app_priv.h"

/* This is the button that is used for toggling the power */
#define BUTTON_GPIO          CONFIG_EXAMPLE_BOARD_BUTTON_GPIO
#define BUTTON_ACTIVE_LEVEL  0

/* This is the GPIO on which the power will be set */
#define OUTPUT_GPIO    3
static bool g_power_state = DEFAULT_POWER;
static bool flag_alarm_timer_created = false;

/* These values correspoind to H,S,V = 120,100,10 */
#define DEFAULT_RED     255
#define DEFAULT_GREEN   0
#define DEFAULT_BLUE    0

static TimerHandle_t alarm_timer;
static bool alarm_state = false;

#define WIFI_RESET_BUTTON_TIMEOUT       3
#define FACTORY_RESET_BUTTON_TIMEOUT    10

static const char *TAG = "app_driver";

static void app_indicator_set(bool state)
{
    if (state) {
        ws2812_led_set_rgb(DEFAULT_RED, DEFAULT_GREEN, DEFAULT_BLUE);
    } else {
        ws2812_led_clear();
    }
}

static void app_indicator_init(void)
{
    ws2812_led_init();
    app_indicator_set(g_power_state);
}
static void push_btn_cb(void *arg)
{
    bool new_state = !g_power_state;
    app_driver_set_state(new_state);
#ifdef CONFIG_EXAMPLE_ENABLE_TEST_NOTIFICATIONS
    /* This snippet has been added just to demonstrate how the APIs esp_rmaker_param_update_and_notify()
     * and esp_rmaker_raise_alert() can be used to trigger push notifications on the phone apps.
     * Normally, there should not be a need to use these APIs for such simple operations. Please check
     * API documentation for details.
     */
    if (new_state) {
        esp_rmaker_param_update_and_notify(
                esp_rmaker_device_get_param_by_name(switch_device, ESP_RMAKER_DEF_POWER_NAME),
                esp_rmaker_bool(new_state));
    } else {
        esp_rmaker_param_update_and_report(
                esp_rmaker_device_get_param_by_name(switch_device, ESP_RMAKER_DEF_POWER_NAME),
                esp_rmaker_bool(new_state));
        esp_rmaker_raise_alert("Switch was turned off");
    }
#else
    esp_rmaker_param_update_and_report(
            esp_rmaker_device_get_param_by_name(switch_device, ESP_RMAKER_DEF_POWER_NAME),
            esp_rmaker_bool(new_state));
#endif
}

static void set_power_state(bool target)
{
    gpio_set_level(OUTPUT_GPIO, target);
    app_indicator_set(target);
}

static void app_alarm_update(TimerHandle_t handle)
{
    if (alarm_state == true)
    {
        set_power_state(false);
        alarm_state = false;
    } else if (alarm_state == false)
    {
        set_power_state(true);
        alarm_state = true;
    } else {
        ESP_LOGI(TAG, "Weird alarm_state: %i, setting to false", alarm_state);
        set_power_state(false);
    }
    
}

void app_driver_init()
{
    button_handle_t btn_handle = iot_button_create(BUTTON_GPIO, BUTTON_ACTIVE_LEVEL);
    if (btn_handle) {
        /* Register a callback for a button tap (short press) event */
        iot_button_set_evt_cb(btn_handle, BUTTON_CB_TAP, push_btn_cb, NULL);
        /* Register Wi-Fi reset and factory reset functionality on same button */
        app_reset_button_register(btn_handle, WIFI_RESET_BUTTON_TIMEOUT, FACTORY_RESET_BUTTON_TIMEOUT);
    }

    /* Configure power */
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = 1,
    };
    io_conf.pin_bit_mask = ((uint64_t)1 << OUTPUT_GPIO);
    /* Configure the GPIO */
    gpio_config(&io_conf);
    app_indicator_init();
}

int IRAM_ATTR app_driver_set_state(bool state) // Error: conflicting types for 'app_driver_set_state'; have 'int(int)
{
    g_power_state = state;
    if(g_power_state == true)
    {
        alarm_timer = xTimerCreate("app_alarm_update_tm", (REPORTING_PERIOD * 1000) / portTICK_PERIOD_MS,
                                pdTRUE, NULL, app_alarm_update);
        flag_alarm_timer_created = true;
        if (alarm_timer) {
            xTimerStart(alarm_timer, 0);
        }
    } else if (g_power_state == false && flag_alarm_timer_created == true)
    {
        int err2 = xTimerDelete(alarm_timer, 1); // xBlockTime set to 1 tick to prevent alarm_timer not deleted before next line, NULL only throws warning for casting type
        set_power_state(false);
        if (err2 != 1) {
            ESP_LOGE(TAG, "Could not xTimerDelete, err: %i", err2);
            vTaskDelay(5000/portTICK_PERIOD_MS);
        }
    } else
    {
        set_power_state(false);
    }
    return ESP_OK;
}

bool app_driver_get_state(void)
{
    return g_power_state;
}