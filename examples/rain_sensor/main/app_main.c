/* Temperature Sensor Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <nvs_flash.h>

#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_types.h>
#include <esp_rmaker_standard_params.h>
#include <esp_rmaker_standard_devices.h>

#include <app_wifi.h>
#include <app_insights.h>

#include <driver/gpio.h>
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#include "app_priv.h"

static const char *TAG = "app_main";

esp_rmaker_device_t *rain_sensor_device;

void app_main()
{
    /* Initialize Application specific hardware drivers and
     * set initial state.
     */
    app_driver_init();

    /* Initialize NVS. */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    /* Initialize Wi-Fi. Note that, this should be called before esp_rmaker_node_init()
     */
    app_wifi_init();
    
    /* Initialize the ESP RainMaker Agent.
     * Note that this should be called after app_wifi_init() but before app_wifi_start()
     * */
    esp_rmaker_config_t rainmaker_cfg = {
        .enable_time_sync = false,
    };
    esp_rmaker_node_t *node = esp_rmaker_node_init(&rainmaker_cfg, "ESP RainMaker Device", "Unknown-Rain_Sensor");
    if (!node) {
        ESP_LOGE(TAG, "Could not initialise node. Aborting!!!");
        vTaskDelay(5000/portTICK_PERIOD_MS);
        abort();
    }

    /* Create a device and add the relevant parameters to it */
    // Error when trying to beautify UI: Guru Meditation Error: Core  0 panic'ed (Load access fault). Exception was unhandled.
    // rain_sensor_device = esp_rmaker_device_create("Device-Rain_Sensor", ESP_RMAKER_DEVICE_LIGHTBULB, NULL);
    // esp_rmaker_param_t *precipitation_param = esp_rmaker_param_create("Precipitation", NULL, esp_rmaker_bool(false), PROP_FLAG_READ);
    // esp_rmaker_param_add_ui_type(precipitation_param, ESP_RMAKER_UI_TEXT);
    // esp_rmaker_device_add_param(rain_sensor_device, precipitation_param);
    rain_sensor_device = esp_rmaker_temp_sensor_device_create("Rain Sensor", ESP_RMAKER_DEVICE_LIGHTBULB, app_get_current_precipitation()); // Changing 2nd arg into ESP_RMAKER_DEVICE_LIGHTBULB doesn't work for temp_sensor_device_create;

    // esp_rmaker_device_add_param(rain_sensor_device, esp_rmaker_name_param_create(ESP_RMAKER_DEF_NAME_PARAM, "Rain Sensor")); // tried to add name of parameter but overwritten by temp_sensor_device_create

    esp_rmaker_node_add_device(node, rain_sensor_device);

    /* Enable OTA */
    esp_rmaker_ota_enable_default();

    /* Enable Insights. Requires CONFIG_ESP_INSIGHTS_ENABLED=y */
    app_insights_enable();

    /* Start the ESP RainMaker Agent */
    esp_rmaker_start();

    /* Start the Wi-Fi.
     * If the node is provisioned, it will start connection attempts,
     * else, it will start Wi-Fi provisioning. The function will return
     * after a connection has been successfully established
     */
    err = app_wifi_start(POP_TYPE_RANDOM);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Could not start Wifi. Aborting!!!");
        vTaskDelay(5000/portTICK_PERIOD_MS);
        abort();
    }
}
