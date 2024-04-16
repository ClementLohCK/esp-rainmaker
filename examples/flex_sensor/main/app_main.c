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

esp_rmaker_device_t *temp_sensor_device; // TODO=: Rename into flex_sensor_device, check app_priv.h too

/* Callback to handle commands received from the RainMaker cloud */
static esp_err_t write_cb(const esp_rmaker_device_t *device, const esp_rmaker_param_t *param,
            const esp_rmaker_param_val_t val, void *priv_data, esp_rmaker_write_ctx_t *ctx)
{
    if (ctx) {
        ESP_LOGI(TAG, "Parameter %s kena called back cause received write request via : %s", esp_rmaker_param_get_name(param), esp_rmaker_device_cb_src_to_str(ctx->src));
    }
    char *device_name = esp_rmaker_device_get_name(device);
    char *param_name = esp_rmaker_param_get_name(param);
    if (strcmp(param_name, "Frequency") == 0) {
        ESP_LOGI(TAG, "Received value = %d for %s - %s",
                val.val.i, device_name, param_name);
        app_driver_set_level(val.val.i);
    } else {
        ESP_LOGI(TAG, "Nothing changed");
    }
    esp_rmaker_param_update_and_report(param, val);
    return ESP_OK;
}

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
    esp_rmaker_node_t *node = esp_rmaker_node_init(&rainmaker_cfg, "ESP RainMaker Device", "Type-Flex_Sensor"); // 2nd arg only appears if got multiple devices within 1 node
    if (!node) {
        ESP_LOGE(TAG, "Could not initialise node. Aborting!!!");
        vTaskDelay(5000/portTICK_PERIOD_MS);
        abort();
    }

    /* Create a device and add the relevant parameters to it */
    temp_sensor_device = esp_rmaker_temp_sensor_device_create("Flex Sensor", NULL, app_get_current_temperature());
    esp_rmaker_device_add_cb(temp_sensor_device, write_cb, NULL);

    /* Create own parameter */
    esp_rmaker_param_t *frequency_param = esp_rmaker_speed_param_create("Frequency", DEFAULT_SPEED);
    esp_rmaker_param_add_ui_type(frequency_param, ESP_RMAKER_UI_SLIDER);
    esp_rmaker_device_add_param(temp_sensor_device, frequency_param);
    // esp_rmaker_param_t *switch_param = esp_rmaker_param_create("Alert", NULL, esp_rmaker_bool(false), PROP_FLAG_READ | PROP_FLAG_WRITE);
    // esp_rmaker_param_add_ui_type(switch_param, ESP_RMAKER_UI_TRIGGER); // Trigger needs to be able to turn false cause when stop raining, Rain_Sensor also need to deactivate Alert; Can try look into Slider for different levels of sampling rate
    // esp_rmaker_device_add_param(temp_sensor_device, switch_param);

    esp_rmaker_node_add_device(node, temp_sensor_device);

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
    };
}
