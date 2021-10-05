/*
 * Created by @georgezhao2010
 * https://github.com/georgezhao2010/esp32_meizu_remoter_gateway/
 * involute@sina.com
 */
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <nvs.h>
#include <nvs_flash.h>
#include "wifi.h"
#include "led.h"
#include "mdnsmd.h"
#include "utils.h"
#include "kernel.h"

#ifdef CONFIG_WIFI_RESET_PROVISIONED
    #define RESTORE_WIFI_CONFIG true
#else
    #define RESTORE_WIFI_CONFIG false
#endif

typedef struct esp_custom_app_desc_t {
	char app_author_name[16];
}esp_custom_app_desc_t ;

const __attribute__((section(".rodata_custom_desc"))) esp_custom_app_desc_t custom_app_desc = {.app_author_name="@georgezhao2010"};

Kernel kernel;

static int g_wifi_reconnect = 0;

void on_wifi_prov_gotip(){
    ESP_LOGI("main", "Successful to provisioning, reboot in 10 seconds");
    Kernel::Restart();
}

void on_wifi_connect(bool connected){
    if(connected){
        ESP_LOGI("main", "WiFi connected");
        g_wifi_reconnect = 0;
        led_set_mode(LED_MODE_ON, 0);
    } 
    else {
        ESP_LOGI("main", "WiFi disconencted");
        g_wifi_reconnect ++;
        if(g_wifi_reconnect >= 30){
            Kernel::Restart();
        }
        led_set_mode(LED_MODE_OFF, 0);
    }
}

bool setup(void){
    esp_err_t ret = nvs_flash_init();
    ESP_LOGI("Init", "Flash initializing");
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    if(ret != ESP_OK){
        ESP_LOGE("Init", "Failed to Flash initialization");
        return false;
    }
    ESP_LOGI("Init", "LED initializing");
    led_init();
    ESP_LOGI("Init", "WiFi initializing");
    if(!wifi_init()){
        ESP_LOGE("Init", "Failed to WIFI initialization");
        return false;
    }
    
    bool provisioned;
    if(!wifi_provisioned(RESTORE_WIFI_CONFIG, &provisioned)){
        ESP_LOGE("Init", "Failed to Get WiFi provision");
        return false;
    }
    if(!provisioned){
        led_set_mode(LED_MODE_SLOW_BLINK, 0);
        start_softap_provisioning(on_wifi_prov_gotip);
        return false;
    } else {
        if(!wifi_init_sta(on_wifi_connect)){
            ESP_LOGE("Init", "Failed to WiFi STA mode initialization");
            return false;
        }
    }
    ESP_LOGI("Init", "mDNS initializing");
    if(!mdnsmd_init()){
        ESP_LOGE("Init", "Failed to mDNS initialization");
    }
    ESP_LOGI("Init", "Kernel processor initializing");
    if(!kernel.Init()){
        ESP_LOGE("Init", "Failed to kernel processor initialization");
        return false;
    }
    ESP_LOGI("Init", "Initialization was completely succeeded");
    return true;
}

extern "C" void app_main(void){
    if(setup()){
        kernel.Loop();
    }
}