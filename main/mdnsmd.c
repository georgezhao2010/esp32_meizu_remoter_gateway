/*
 * Created by @georgezhao2010
 * https://github.com/georgezhao2010/esp32_meizu_remoter_gateway/
 * involute@sina.com
 */
#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_ota_ops.h>
#include <mdns.h>
#include "mdnsmd.h"

#define LOG_TAG "mdnsmd"

char sirialno[7];
const esp_app_desc_t * app_desc;

bool mdnsmd_init(void)
{
    uint8_t mac[6];
    
    esp_err_t ret = esp_read_mac(mac, ESP_MAC_WIFI_STA);
    if(ret != ESP_OK){
        ESP_LOGE(LOG_TAG, "Failed to get the MAC address");
        return false;
    }
    for(int n = 3; n < 6; n++){
        mac[n] ^= 0x55;
    }
    sprintf(sirialno, "%02X%02X%02X", mac[3], mac[4], mac[5]);
    ret = mdns_init();
    if(ret != ESP_OK){
        ESP_LOGE(LOG_TAG, "Failed to mDNS initialization");
        return false;
    }
    ret = mdns_hostname_set(sirialno);
    if(ret != ESP_OK){
        ESP_LOGE(LOG_TAG, "Failed to set mDNS host name");
        return false;
    }
    ESP_LOGI(LOG_TAG, "mDNS host name set to: [%s]", sirialno);
    ret = mdns_instance_name_set(sirialno);
    if(ret != ESP_OK){
        ESP_LOGE(LOG_TAG, "Failed to set mDNS instance name");
        return false;
    }
    ESP_LOGI(LOG_TAG, "mDNS instance name set to: [%s]", sirialno);
    app_desc = esp_ota_get_app_description();
    if(app_desc == NULL){
        ESP_LOGE(LOG_TAG, "Failed to App description");
        return false;
    }
    mdns_txt_item_t serviceTxtData[2] = {
        {"serialno", sirialno},
        {"version", app_desc->version}
    };
    ret = mdns_service_add(NULL, CONFIG_MDNS_SERVICENAME, "_tcp", CONFIG_SOCKET_LISTEN_PORT, serviceTxtData, 2);
    if(ret != ESP_OK){
        ESP_LOGE(LOG_TAG, "Failed to add mDNS service");
        return false;
    }
    
    return true;
}

const char * mdns_get_serial(){
    return sirialno;
}

const char * mdns_get_version(){
    return app_desc->version;
}