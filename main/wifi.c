/*
 * Created by @georgezhao2010
 * https://github.com/georgezhao2010/esp32_meizu_remoter_gateway/
 * involute@sina.com
 */

#include <string.h>
#include <esp_log.h>
#include <esp_err.h>
#include <esp_wifi.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <esp_event.h>
#include <esp_timer.h>
#include <protocomm.h>
#include <protocomm_httpd.h>
#include <protocomm_security0.h>
#include <protocomm_security1.h>
#include <wifi_provisioning/wifi_config.h>
#include "wifi.h"
#include "utils.h"

#define LOG_TAG "wifi"

struct app_prov_data {
    protocomm_t *pc;
    int security;
    const protocomm_security_pop_t *pop;
    esp_timer_handle_t timer;
    wifi_prov_sta_state_t wifi_state;
    wifi_prov_sta_fail_reason_t wifi_disconnect_reason;
};

static struct app_prov_data *g_prov = NULL;

static esp_err_t get_status_handler(wifi_prov_config_get_data_t *resp_data, wifi_prov_ctx_t **ctx);
static esp_err_t set_config_handler(const wifi_prov_config_set_data_t *req_data, wifi_prov_ctx_t **ctx);
static esp_err_t apply_config_handler(wifi_prov_ctx_t **ctx);

wifi_prov_config_handlers_t wifi_prov_handlers = {
    .get_status_handler   = get_status_handler,
    .set_config_handler   = set_config_handler,
    .apply_config_handler = apply_config_handler,
    .ctx = NULL
};

struct wifi_prov_ctx {
    wifi_config_t wifi_cfg;
};

static bool wifiConnected = false;

bool prov_start_softap_provisioning(const char *ssid, const char *pass,
                                    int security, const protocomm_security_pop_t *pop);

on_connect on_connect_cb = NULL;
on_prov_gotip on_prov_gotip_cb = NULL;

bool wifi_connected(){
    return wifiConnected;
}

static void prov_event_handler(void* handler_arg, esp_event_base_t event_base,
                                   int32_t event_id, void* event_data){
    if(!g_prov) {
        return;
    }

    if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(LOG_TAG, "STA Start");
        g_prov->wifi_state = WIFI_PROV_STA_CONNECTING;
    } 
    else if(event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ESP_LOGI(LOG_TAG, "STA Got IP");
        if(on_prov_gotip_cb != NULL){
            (*on_prov_gotip_cb)(true);
        }
        g_prov->wifi_state = WIFI_PROV_STA_CONNECTED;
    } 
    else if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGE(LOG_TAG, "STA Disconnected");
        g_prov->wifi_state = WIFI_PROV_STA_DISCONNECTED;
        wifi_event_sta_disconnected_t* disconnected = (wifi_event_sta_disconnected_t*) event_data;
        ESP_LOGE(LOG_TAG, "Disconnect reason : %d", disconnected->reason);
        switch (disconnected->reason) {
            case WIFI_REASON_AUTH_EXPIRE:
            case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
            case WIFI_REASON_BEACON_TIMEOUT:
            case WIFI_REASON_AUTH_FAIL:
            case WIFI_REASON_ASSOC_FAIL:
            case WIFI_REASON_HANDSHAKE_TIMEOUT:
                ESP_LOGI(LOG_TAG, "STA Auth Error");
                g_prov->wifi_disconnect_reason = WIFI_PROV_STA_AUTH_ERROR;
                break;
            case WIFI_REASON_NO_AP_FOUND:
                ESP_LOGI(LOG_TAG, "STA AP Not found");
                g_prov->wifi_disconnect_reason = WIFI_PROV_STA_AP_NOT_FOUND;
                break;
            default:
                g_prov->wifi_state = WIFI_PROV_STA_CONNECTING;
        }
        esp_wifi_connect();
    }
}

static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data){
    static int s_retry_num = 0;

    if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } 
    else if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(LOG_TAG,"Failed to connect to the AP");
        if(on_connect_cb != NULL){
            (*on_connect_cb)(false);
        }
        wifiConnected = false;
        delay(5000);
        //if(s_retry_num < CONFIG_WIFI_AP_RECONN_ATTEMPTS) {
        esp_wifi_connect();
        s_retry_num++;
        ESP_LOGI(LOG_TAG, "Retry to connect to the AP");
        //}
    } 
    else if(event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(LOG_TAG, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        wifiConnected = true;
        if(on_connect_cb != NULL){
            (*on_connect_cb)(true);
        }
    }
}

bool wifi_init_sta(on_connect cb){
    on_connect_cb = cb;
    esp_err_t ret = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, event_handler, NULL);
    if(ret != ESP_OK){
        ESP_LOGE(LOG_TAG, "Failed to ESP event handler register: %s", esp_err_to_name(ret));
        return false;
    }
    ret = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, event_handler, NULL);
    if(ret != ESP_OK){
        ESP_LOGE(LOG_TAG, "Failed to ESP event handler register: %s", esp_err_to_name(ret));
        return false;
    }
    ret = esp_wifi_set_mode(WIFI_MODE_STA);
    if(ret != ESP_OK){
        ESP_LOGE(LOG_TAG, "Failed to set Wifi STA mode: %s", esp_err_to_name(ret));
        return false;
    }
    ret = esp_wifi_start();
    if(ret != ESP_OK){
        ESP_LOGE(LOG_TAG, "Failed to start Wifi on STA mode: %s", esp_err_to_name(ret));
        return false;
    }
    return true;
}

bool wifi_init(){
    esp_err_t ret = esp_netif_init();
    if(ret != ESP_OK){
        ESP_LOGE(LOG_TAG, "Networking stack initialization failed: %s", esp_err_to_name(ret));
        return false;
    }
    ret = esp_event_loop_create_default();
    if(ret != ESP_OK){
        ESP_LOGE(LOG_TAG, "Event loop initialization failed: %s", esp_err_to_name(ret));
        return false;
    }
    esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ret = esp_wifi_init(&cfg);
    if(ret != ESP_OK){
        ESP_LOGE(LOG_TAG, "ESP WiFi initialization failed: %s", esp_err_to_name(ret));
        return false;
    }
    return true;
}

bool wifi_provisioned(bool restore, bool *provisioned){
    *provisioned = false;
    if(restore){
        esp_wifi_restore();
        return true;
    }
    wifi_config_t wifi_cfg;
    esp_err_t ret = esp_wifi_get_config(WIFI_IF_STA, &wifi_cfg);
    if(ret != ESP_OK){
        ESP_LOGE(LOG_TAG, "Get WiFi config failed: %s", esp_err_to_name(ret));
        return false;
    }

    if(strlen((const char*) wifi_cfg.sta.ssid)) {
        *provisioned = true;
        ESP_LOGI(LOG_TAG, "Found ssid %s",     (const char*) wifi_cfg.sta.ssid);
        ESP_LOGI(LOG_TAG, "Found password %s", (const char*) wifi_cfg.sta.password);
    }
    return true;
}

bool start_softap_provisioning(on_prov_gotip cb){
    on_prov_gotip_cb = cb;
    int security = 1;
    const protocomm_security_pop_t * pop = NULL;
#ifdef CONFIG_WIFI_SOFTAP_USE_POP
    const static protocomm_security_pop_t app_pop = {
        .data = (uint8_t *) CONFIG_WIFI_SOFTAP_POP,
        .len = (sizeof(CONFIG_WIFI_SOFTAP_POP)-1)
    };
    pop = &app_pop;
#endif
    const char *ssid = NULL;
    uint8_t eth_mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
    char ssid_with_mac[33];
    snprintf(ssid_with_mac, sizeof(ssid_with_mac), "ESP_%02X%02X%02X",
                eth_mac[3], eth_mac[4], eth_mac[5]);

    ssid = ssid_with_mac;
    return prov_start_softap_provisioning(ssid, CONFIG_WIFI_SOFTAP_PASS, security,  pop);
}

static esp_err_t start_wifi_ap(const char *ssid, const char *pass){
    wifi_config_t wifi_config = {
        .ap = {
            .max_connection = 5,
        },
    };

    strlcpy((char *) wifi_config.ap.ssid, ssid, sizeof(wifi_config.ap.ssid));

    if(strlen(pass) == 0) {
        memset(wifi_config.ap.password, 0, sizeof(wifi_config.ap.password));
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    } 
    else{
        strlcpy((char *) wifi_config.ap.password, pass, sizeof(wifi_config.ap.password));
        wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    }
    esp_err_t ret = esp_wifi_set_mode(WIFI_MODE_AP);
    if(ret != ESP_OK) {
        ESP_LOGE(LOG_TAG, "Failed to set WiFi AP mode: %s", esp_err_to_name(ret));
        return ret;
    }
    ret = esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    if(ret != ESP_OK) {
        ESP_LOGE(LOG_TAG, "Failed to set WiFi config: %s", esp_err_to_name(ret));
        return ret;
    }
    ret = esp_wifi_start();
    if(ret != ESP_OK) {
        ESP_LOGE(LOG_TAG, "Failed to start Wifi on AP mode: %s", esp_err_to_name(ret));
        return ret;
    }

    return ESP_OK;
}

static esp_err_t prov_start_service(void){
    g_prov->pc = protocomm_new();
    if(g_prov->pc == NULL) {
        ESP_LOGE(LOG_TAG, "Failed to create new protocomm instance");
        return ESP_FAIL;
    }
    protocomm_httpd_config_t pc_config = {
        .ext_handle_provided = false,
        .data = {
            .config = PROTOCOMM_HTTPD_DEFAULT_CONFIG()
        }
    };
    if(protocomm_httpd_start(g_prov->pc, &pc_config) != ESP_OK) {
        ESP_LOGE(LOG_TAG, "Failed to start protocomm HTTP server");
        return ESP_FAIL;
    }
    protocomm_set_version(g_prov->pc, "proto-ver", "V0.1");
    if(g_prov->security == 0) {
        protocomm_set_security(g_prov->pc, "prov-session", &protocomm_security0, NULL);
    } else if(g_prov->security == 1) {
        protocomm_set_security(g_prov->pc, "prov-session", &protocomm_security1, g_prov->pop);
    }
    if(protocomm_add_endpoint(g_prov->pc, "prov-config",
                               wifi_prov_config_data_handler,
                               (void *) &wifi_prov_handlers) != ESP_OK) {
        ESP_LOGE(LOG_TAG, "Failed to set provisioning endpoint");
        protocomm_httpd_stop(g_prov->pc);
        return ESP_FAIL;
    }
    return ESP_OK;
}

bool prov_start_softap_provisioning(const char *ssid, const char *pass,
                                    int security, const protocomm_security_pop_t *pop){
    g_prov = (struct app_prov_data *) calloc(1, sizeof(struct app_prov_data));
    if(!g_prov) {
        ESP_LOGI(LOG_TAG, "Unable to allocate prov data");
        return false;
    }
    g_prov->pop = pop;
    g_prov->security = security;

    esp_err_t ret = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, prov_event_handler, NULL);
    if(ret != ESP_OK) {
        ESP_LOGE(LOG_TAG, "Failed to register WiFi event handler: %s", esp_err_to_name(ret));
        return false;
    }

    ret = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, prov_event_handler, NULL);
    if(ret != ESP_OK) {
        ESP_LOGE(LOG_TAG, "Failed to register IP event handler: %s", esp_err_to_name(ret));
        return false;
    }

    ret = start_wifi_ap(ssid, pass);
    if(ret != ESP_OK) {
        ESP_LOGE(LOG_TAG, "Failed to start WiFi AP: %s", esp_err_to_name(ret));
        return false;
    }

    ret = prov_start_service();
    if(ret != ESP_OK) {
        ESP_LOGE(LOG_TAG, "Failed to start provisioning app: %s", esp_err_to_name(ret));
        return false;
    }

    ESP_LOGI(LOG_TAG, "SoftAP Provisioning started with SSID '%s', Password '%s'", ssid, pass);
    return true;
}

static wifi_config_t *get_config(wifi_prov_ctx_t **ctx){
    return (*ctx ? &(*ctx)->wifi_cfg : NULL);
}

static wifi_config_t *new_config(wifi_prov_ctx_t **ctx){
    free(*ctx);
    (*ctx) = (wifi_prov_ctx_t *) calloc(1, sizeof(wifi_prov_ctx_t));
    return get_config(ctx);
}

static void free_config(wifi_prov_ctx_t **ctx){
    free(*ctx);
    *ctx = NULL;
}

esp_err_t prov_get_wifi_state(wifi_prov_sta_state_t* state){
    if(g_prov == NULL || state == NULL) {
        return ESP_FAIL;
    }

    *state = g_prov->wifi_state;
    return ESP_OK;
}

esp_err_t prov_get_wifi_disconnect_reason(wifi_prov_sta_fail_reason_t* reason){
    if(g_prov == NULL || reason == NULL) {
        return ESP_FAIL;
    }

    if(g_prov->wifi_state != WIFI_PROV_STA_DISCONNECTED) {
        return ESP_FAIL;
    }

    *reason = g_prov->wifi_disconnect_reason;
    return ESP_OK;
}

static esp_err_t get_status_handler(wifi_prov_config_get_data_t *resp_data, wifi_prov_ctx_t **ctx){
    memset(resp_data, 0, sizeof(wifi_prov_config_get_data_t));

    if(prov_get_wifi_state(&resp_data->wifi_state) != ESP_OK) {
        ESP_LOGW(LOG_TAG, "Prov app not running");
        return ESP_FAIL;
    }

    if(resp_data->wifi_state == WIFI_PROV_STA_CONNECTED) {
        ESP_LOGI(LOG_TAG, "Connected state");
        esp_netif_ip_info_t ip_info;
        esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"), &ip_info);
        esp_ip4addr_ntoa(&ip_info.ip, resp_data->conn_info.ip_addr, sizeof(resp_data->conn_info.ip_addr));
        wifi_ap_record_t ap_info;
        esp_wifi_sta_get_ap_info(&ap_info);
        memcpy(resp_data->conn_info.bssid, (char *)ap_info.bssid, sizeof(ap_info.bssid));
        memcpy(resp_data->conn_info.ssid,  (char *)ap_info.ssid,  sizeof(ap_info.ssid));
        resp_data->conn_info.channel   = ap_info.primary;
        resp_data->conn_info.auth_mode = ap_info.authmode;
    } 
    else if(resp_data->wifi_state == WIFI_PROV_STA_DISCONNECTED) {
        ESP_LOGI(LOG_TAG, "Disconnected state");
        prov_get_wifi_disconnect_reason(&resp_data->fail_reason);
    } 
    else{
        ESP_LOGI(LOG_TAG, "Connecting state");
    }
    return ESP_OK;
}

static esp_err_t set_config_handler(const wifi_prov_config_set_data_t *req_data, wifi_prov_ctx_t **ctx){
    wifi_config_t *wifi_cfg = get_config(ctx);
    if(wifi_cfg) {
        free_config(ctx);
    }
    wifi_cfg = new_config(ctx);
    if(!wifi_cfg) {
        ESP_LOGE(LOG_TAG, "Unable to alloc wifi config");
        return ESP_FAIL;
    }
    ESP_LOGI(LOG_TAG, "WiFi Credentials Received : \n\tssid %s \n\tpassword %s",
             req_data->ssid, req_data->password);
    const size_t ssid_len = strnlen(req_data->ssid, sizeof(wifi_cfg->sta.ssid));
    memset(wifi_cfg->sta.ssid, 0, sizeof(wifi_cfg->sta.ssid));
    memcpy(wifi_cfg->sta.ssid, req_data->ssid, ssid_len);

    strlcpy((char *) wifi_cfg->sta.password, req_data->password, sizeof(wifi_cfg->sta.password));
    return ESP_OK;
}

esp_err_t prov_configure_sta(wifi_config_t *wifi_cfg){
    if(esp_wifi_set_mode(WIFI_MODE_APSTA) != ESP_OK) {
        ESP_LOGE(LOG_TAG, "Failed to set WiFi mode");
        return ESP_FAIL;
    }
    if(esp_wifi_set_config(WIFI_IF_STA, wifi_cfg) != ESP_OK) {
        ESP_LOGE(LOG_TAG, "Failed to set WiFi configuration");
        return ESP_FAIL;
    }
    if(esp_wifi_start() != ESP_OK) {
        ESP_LOGE(LOG_TAG, "Failed to restart WiFi");
        return ESP_FAIL;
    }
    if(esp_wifi_connect() != ESP_OK) {
        ESP_LOGE(LOG_TAG, "Failed to connect WiFi");
        return ESP_FAIL;
    }
    if(g_prov) {
        g_prov->wifi_state = WIFI_PROV_STA_CONNECTING;
    }
    return ESP_OK;
}

static esp_err_t apply_config_handler(wifi_prov_ctx_t **ctx){
    wifi_config_t *wifi_cfg = get_config(ctx);
    if(!wifi_cfg) {
        ESP_LOGE(LOG_TAG, "WiFi config not set");
        return ESP_FAIL;
    }
    prov_configure_sta(wifi_cfg);
    ESP_LOGI(LOG_TAG, "WiFi Credentials Applied");
    free_config(ctx);
    return ESP_OK;
}