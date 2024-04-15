/* GPIO Example

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
// #include <esp_rmaker_scenes.h>

#include <app_wifi.h>
#include <app_insights.h>

// #include <json_generator.h>
// #include <time.h>

#include "app_priv.h"

static const char *TAG = "app_main";

/* Callback to handle commands received from the RainMaker cloud */
static esp_err_t write_cb(const esp_rmaker_device_t *device, const esp_rmaker_param_t *param,
            const esp_rmaker_param_val_t val, void *priv_data, esp_rmaker_write_ctx_t *ctx)
{
    if (ctx) {
        ESP_LOGI(TAG, "Received write request via : %s", esp_rmaker_device_cb_src_to_str(ctx->src));
    }
    if (app_driver_set_gpio(esp_rmaker_param_get_name(param), val.val.b) == ESP_OK) {
        esp_rmaker_param_update_and_report(param, val);
    }
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
    esp_rmaker_node_t *node = esp_rmaker_node_init(&rainmaker_cfg, "5ESP RainMaker Device5", "Test-Alarm");
    if (!node) {
        ESP_LOGE(TAG, "Could not initialise node. Aborting!!!");
        vTaskDelay(5000/portTICK_PERIOD_MS);
        abort();
    }

    /* Create a device and add the relevant parameters to it */
    esp_rmaker_device_t *gpio_device = esp_rmaker_device_create("Test-Alarm", NULL, NULL);
    esp_rmaker_device_add_cb(gpio_device, write_cb, NULL);

    esp_rmaker_param_t *red_param = esp_rmaker_param_create("Red", NULL, esp_rmaker_bool(false), PROP_FLAG_READ | PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(red_param, ESP_RMAKER_UI_TOGGLE);
    esp_rmaker_device_add_param(gpio_device, red_param);

    esp_rmaker_param_t *green_param = esp_rmaker_param_create("Green", NULL, esp_rmaker_bool(false), PROP_FLAG_READ | PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(green_param, ESP_RMAKER_UI_TOGGLE);
    esp_rmaker_device_add_param(gpio_device, green_param);

    esp_rmaker_param_t *blue_param = esp_rmaker_param_create("Blue", NULL, esp_rmaker_bool(false), PROP_FLAG_READ | PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(blue_param, ESP_RMAKER_UI_TOGGLE);
    esp_rmaker_device_add_param(gpio_device, blue_param);

    esp_rmaker_node_add_device(node, gpio_device);

    /* ****** <TODO=: Testing Scenes ****** */
    /* Create the service using esp_rmaker_service_create(). However, note that a service uses esp_rmaker_device_t
    * as the data type, since it is structurally same as a device.
    */
    // esp_rmaker_device_t *diag_service = esp_rmaker_service_create("Diagnostics","my.service.diag", NULL);

    // "invalid storage class for function 'diag_write_cb'" problem with adding cb function for scene_service so commenting this part out
    // static esp_err_t diag_write_cb(const esp_rmaker_device_t *device, const esp_rmaker_param_t *param, const esp_rmaker_param_val_t val, void *priv_data, esp_rmaker_write_ctx_t *ctx)
    // {
    //     /* This ctx check is just to find if the request was received via Cloud, Local network or Schedule.
    //     * Having this is not required, but there could be some cases wherein specific operations may be allowed
    //     * only via specific channels (like only Local network), where this would be useful.
    //     */
    //     if (ctx) {
    //         ESP_LOGI(TAG, "Received write request via : %s", esp_rmaker_device_cb_src_to_str(ctx->src));
    //     }

    //     /* Check if the write is on the "Trigger" parameter. We aren't really checking true/false as that
    //     * is not much of a concern in this context. But you can add checks on the values too.
    //     */
    //     if (strcmp(esp_rmaker_param_get_name(param), "Trigger") == 0) {
    //         /* Here we start some dummy diagnostics and populate the appropriate values to be passed
    //         * to "Timestamp" and "Data".
    //         */
    //         ESP_LOGI(TAG, "Starting Diagnostics");
    //         time_t current_timestamp = 0;
    //         time(&current_timestamp);
    //         char buf[100] = {0};
    //         json_gen_str_t jstr;
    //         json_gen_str_start(&jstr, buf, sizeof(buf), NULL, NULL);
    //         json_gen_start_object(&jstr);
    //         json_gen_obj_set_bool(&jstr, "diag1", true);
    //         json_gen_obj_set_int(&jstr, "diag2", 30);
    //         json_gen_obj_set_float(&jstr, "diag3", 54.1643);
    //         json_gen_obj_set_string(&jstr, "diag4", "diag");
    //         json_gen_end_object(&jstr);
    //         json_gen_str_end(&jstr);
    
    //         /* The values are reported by updating appropriate parameters */
    //         esp_rmaker_param_update_and_report(esp_rmaker_device_get_param_by_name(device, "Data"),
    //                     esp_rmaker_obj(buf));
    //         esp_rmaker_param_update_and_report(esp_rmaker_device_get_param_by_name(device, "Timestamp"),
    //                 esp_rmaker_int((int)current_timestamp));
    //     }
    // return ESP_OK;
    // }

    // /* Register the write callback. Read callback would normally be NULL */
    // esp_rmaker_device_add_cb(diag_service, diag_write_cb, NULL);
    
    // /* Create and add paramaters of various types as applicable.
    // * Parameter types (like my.param.diag-trigger) are not mandatory, but useful to have.
    // */
    // esp_rmaker_device_add_param(diag_service, esp_rmaker_param_create("Trigger", "my.param.diag-trigger", esp_rmaker_bool(false), PROP_FLAG_WRITE));
    // esp_rmaker_device_add_param(diag_service, esp_rmaker_param_create("Timestamp", "my.param.diag-timestamp", esp_rmaker_int(0), PROP_FLAG_READ));
    // esp_rmaker_device_add_param(diag_service, esp_rmaker_param_create("Data", "my.param.diag-data", esp_rmaker_obj("{}"), PROP_FLAG_READ));

    // /* Add the service to the node */
    // esp_rmaker_node_add_device(node, diag_service);

    // esp_rmaker_device_t *scene_service = esp_rmaker_service_create("Scene","esp.service.scenes", NULL);

    // def scene_write_cb was here

    // /* Register the write callback. Read callback would normally be NULL */
    // esp_rmaker_device_add_cb(scene_service, scene_write_cb, NULL);

    // /* Create and add paramaters of various types as applicable.
    //  * Parameter types (like my.param.diag-trigger) are not mandatory, but useful to have.
    //  */
    // esp_rmaker_device_add_param(scene_service, esp_rmaker_param_create("Trigger", NULL, esp_rmaker_bool(false), PROP_FLAG_READ | PROP_FLAG_WRITE));

    // /* Add the service to the node */
    // esp_rmaker_node_add_device(node, scene_service);
    /* ****** </TODO=: Testing Scenes> ****** */

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
