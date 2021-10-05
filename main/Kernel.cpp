/*
 * Created by @georgezhao2010
 * https://github.com/georgezhao2010/esp32_meizu_remoter_gateway/
 * involute@sina.com
 */
#include <driver/gpio.h>
#include <esp_log.h>
#include <cjson.h>
#include <string.h>
#include <list>
#include <esp_pthread.h>
#include "led.h"
#include "utils.h"
#include "wifi.h"
#include "nvsstorage.h"
#include "mdnsmd.h"
#include "Kernel.h"

#define BLE_MAX_FIAILED_TIMES 5
#define LOG_TAG "Kernel"

const char * template_config_info = "{\"type\":\"config_info\",\"data\":{\"version\":\"%s\",\"serialno\":\"%s\", \"update_interval\":%d}}";
const char * boardcast_template_available = "{\"type\":\"update\",\"data\":{\"device\":\"%s\",\"available\":1,\"status\":%s}}";
const char * boardcast_template_unavailable = "{\"type\":\"update\",\"data\":{\"device\":\"%s\",\"available\":0}}";
const char * template_devices = "{\"type\":\"devices\",\"data\":{\"devices\":%s}}";
const char * template_device = "{\"device\":\"%s\",\"available\":1,\"status\":%s},";
const char * boardcast_template_setinterval = "{\"type\":\"setinterval\",\"data\":{\"update_interval\":%d}}";
const char * boardcast_template_removebind = "{\"type\":\"removebind\",\"data\":{\"device\":\"%s\"}}";
const char * bind_mode_successful = "{\"type\":\"bind\",\"data\":{\"status\":\"1\"}}";
const char * bind_mode_failed = "{\"type\":\"bind\",\"data\":{\"status\":\"0\"}}";

struct jud_param{
    Kernel * pKernel;
    MEIZURemoterInfo * pInfo;
};

Kernel::Kernel(){
    m_mode = KernelMode::Normal;
    m_timertik = 0;
}

Kernel::~Kernel(){

}

void Kernel::task_Restart(void * param){
    led_set_mode(LED_MODE_FAST_BLINK, 0);
    delay(10000);
    esp_restart();
}

bool Kernel::Init(){
    gpio_set_direction(GPIO_NUM_0, GPIO_MODE_INPUT);
    ESP_LOGI(LOG_TAG, "BLE initializing");
    if(!GATTDevice::Init(this, OnBindModeChanged, OnDeviceFound, OnRSSIRead)){
        ESP_LOGE(LOG_TAG, "Failed to BLE initialization");
        return false;
    }
    int8_t threshold;
    if(nvs_read_bindthrld(&threshold)){
        GATTDevice::SetBindThreshold(threshold);
        ESP_LOGI(LOG_TAG, "Bind RSSI threshold set to %d", threshold);
    }
    m_pRemoterInterface = new MEIZURemoter();
    uint8_t count;
    if(nvs_read_addrcount(&count)){
        esp_bd_addr_t * pAddres = (esp_bd_addr_t *)malloc(sizeof(esp_bd_addr_t) * count);
        size_t len = count * ESP_BD_ADDR_LEN;
        nvs_read_addrs((void *)pAddres, &len);
        for(int n = 0; n < count; n++){
            MEIZURemoterInfo * pInfo = new MEIZURemoterInfo();
            pInfo->m_addr = BLEAddress(pAddres[n]);
            m_mapInfos.insert(std::pair<BLEAddress, MEIZURemoterInfo*>(pInfo->m_addr, pInfo));
            ESP_LOGI(LOG_TAG, "Binded device %s loaded", pInfo->m_addr.toString().c_str());
        }
        free(pAddres);
    }
    UpdateAllDevices(this);
    if(!nvs_read_interval(&m_nUpdateInterval)){
        m_nUpdateInterval = CONFIG_DEFAULT_UPDATE_INTERVAL;
    }
    m_server.SetCallback(this, OnClientMessage);
    ESP_LOGI(LOG_TAG, "Start Listening on port %d", CONFIG_SOCKET_LISTEN_PORT);
    if(!m_server.Listen(CONFIG_SOCKET_LISTEN_PORT)){
        ESP_LOGE(LOG_TAG, "Failed to listening");
        return false;
    }
    esp_pthread_cfg_t cfg = esp_pthread_get_default_config();
    cfg.stack_size = (4 * 1024);
    esp_pthread_set_cfg(&cfg);
    esp_timer_create_args_t timer_conf = {
        .callback = OnTimer,
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "update_device_status",
        .skip_unhandled_events = true
    };
    esp_err_t ret = esp_timer_create(&timer_conf, &m_timer);
    if (ret != ESP_OK) {
        ESP_LOGE(LOG_TAG, "Failed to create timer: %s", esp_err_to_name(ret));
        return false;
    }
    return true;
}

void Kernel::Loop(){
    uint16_t counter = 0;
    esp_timer_start_periodic(m_timer, 60000 * 1000U); // 1 minute
    while(true){
        delay(200);
        if(gpio_get_level(GPIO_NUM_0) == 0){
            counter ++;
            if(counter == 10 && m_mode == KernelMode::Normal){
                if(GATTDevice::StartScan()){
                    m_mode = KernelMode::ReadyBind;
                }
            } else if (counter == 40) {
                counter = 0;
                bool pro;
                wifi_provisioned(true, &pro);
                nvs_clear_addrcount();
                nvs_write_interval(0);
                ESP_LOGI(LOG_TAG, "Config reset, reboot in 10 seconds");
                Restart();
            }
        } else {
            counter = 0;
        }
    }
}

void * Kernel::UpdateAllDevices(void * param){
    Kernel * pKernel = (Kernel *)param;
    if(pKernel->m_mode == KernelMode::Normal){
        pKernel->m_lock.Lock();
        for(auto it: pKernel->m_mapInfos){
            MEIZURemoterInfo * pInfo = it.second;
            if(pInfo->m_update <= BLE_MAX_FIAILED_TIMES){
                if(pInfo->m_update <= 0){
                    pInfo->m_update = 1;
                }
                pKernel->m_pRemoterInterface->ReadAllInfo(pInfo);
                led_set_mode(LED_MODE_FAST_BLINK, 1);
                pKernel->BoardcastInfo(pInfo);
            }
        }
        pKernel->m_lock.Unlock();
    }
    return nullptr;
}

void *  Kernel::JustUpdateDevices(void * param){
    struct jud_param * pParam = (struct jud_param *)param;
    if(pParam->pKernel->m_pRemoterInterface->ReadAllInfo(pParam->pInfo)){
        pParam->pKernel->BoardcastInfo(pParam->pInfo);
    }
    delete pParam;
    return nullptr;
}

void Kernel::OnTimer(void * param){
    Kernel * pKernel = (Kernel *)param;
    pKernel->m_timertik ++;
    if(pKernel->m_timertik >= pKernel->m_nUpdateInterval){
        pKernel->m_timertik = 0;
        pthread_t tid;
        pthread_create(&tid, NULL, UpdateAllDevices, pKernel);
        pthread_detach(tid);
    }
}

void Kernel::OnBindModeChanged(void * pCaller, bool bBind){
    Kernel * pKernel = (Kernel *)pCaller;
    if(bBind){
        pKernel->m_mode = KernelMode::Bind;
        ESP_LOGI(LOG_TAG, "Binding mode started");
        led_set_mode(LED_MODE_SLOW_BLINK, 0);
    } else {
        pKernel->m_mode = KernelMode::Normal;
        ESP_LOGI(LOG_TAG, "Binding mode stoped");
        if(wifi_connected()){
            led_set_mode(LED_MODE_ON, 0);
        }
        else{
            led_set_mode(LED_MODE_OFF, 0);
        }
    }
}

void Kernel::OnRSSIRead(void * pCaller, esp_bd_addr_t * addr, int8_t rssi){
    Kernel * pKernel = (Kernel *)pCaller;
    BLEAddress bleaddr(*addr);
    auto it = pKernel->m_mapInfos.find(bleaddr);
    if(it != pKernel->m_mapInfos.end()){
        it->second->m_rssi = rssi;
    }
}

void Kernel::OnDeviceFound(void * pCaller, esp_bd_addr_t * addr){
    Kernel * pKernel = (Kernel *)pCaller;
    
    BLEAddress bleaddr(*addr);
    MEIZURemoterInfo * pExistInfo = nullptr;
    auto it = pKernel->m_mapInfos.find(bleaddr);
    if(it == pKernel->m_mapInfos.end()){
        pExistInfo = nullptr;
    } else {
        pExistInfo = it->second;
    }
    if(pExistInfo == nullptr){
        led_set_mode(LED_MODE_FAST_BLINK, 2);
        GATTDevice::StopScan();
        ESP_LOGI(LOG_TAG, "Found a new device at %s, prepare to pairing", bleaddr.toString().c_str());
        MEIZURemoterInfo * pInfo = new MEIZURemoterInfo();
        pInfo->m_addr = bleaddr;
        pKernel->m_lock.Lock();
        pKernel->m_mapInfos.insert(std::pair<BLEAddress, MEIZURemoterInfo*>(pInfo->m_addr, pInfo));
        pKernel->SaveAddress();
        pKernel->m_lock.Unlock();
        struct jud_param * pParam = new(struct jud_param);
        pParam->pKernel = pKernel;
        pParam->pInfo = pInfo;
        pthread_t tid;
        pthread_create(&tid, NULL, JustUpdateDevices, pParam);
        pthread_detach(tid);
    } else {
        if(pExistInfo->m_update < BLE_MAX_FIAILED_TIMES){
            ESP_LOGW(LOG_TAG, "Found a binded device at %s", bleaddr.toString().c_str());
        } else {
            led_set_mode(LED_MODE_FAST_BLINK, 2);
            GATTDevice::StopScan();
            pExistInfo->m_update = 1;
            struct jud_param * pParam = new(struct jud_param);
            pParam->pKernel = pKernel;
            pParam->pInfo = pExistInfo;
            pthread_t tid;
            pthread_create(&tid, NULL, JustUpdateDevices, pParam);
            pthread_detach(tid);
        }
    }
}

void Kernel::BoardcastInfo(MEIZURemoterInfo * pInfo){
    if(pInfo->m_update == 0){
        char * json_status = pInfo->serialize_to_json();
        ESP_LOGI(LOG_TAG, "successful to update device at %s: %s", pInfo->m_addr.toString().c_str(), json_status);
        int len = strlen(boardcast_template_available) + strlen(json_status) + 18;
        char * message = (char *)malloc(len);
        sprintf(message, boardcast_template_available, pInfo->m_addr.toString().c_str(), json_status);
        m_server.Boardcast(message, strlen(message));
        free(message);
    } else {
        ESP_LOGE(LOG_TAG, "Failed to update device at %s for %d times", pInfo->m_addr.toString().c_str(), pInfo->m_update);
        pInfo->m_update ++;
        if(pInfo->m_update > BLE_MAX_FIAILED_TIMES){
            ESP_LOGW(LOG_TAG, "Failed to update device at %s for many times and will be marked as unavailable.", pInfo->m_addr.toString().c_str());
        }
        int len = strlen(boardcast_template_available) + 18;
        char * message = (char *)malloc(len);
        sprintf(message, boardcast_template_unavailable, pInfo->m_addr.toString().c_str());
        m_server.Boardcast(message, strlen(message));
        free(message);
    }
}

void Kernel::OnClientMessage(void * pCaller, SocketClient * pClient, char * pMessage, int nLen){
    Kernel * pKernel = (Kernel *)pCaller;
    ESP_LOGD(LOG_TAG, "Received message from %s:%d - %s", pClient->GetPeerAddr(), pClient->GetPeerPort(), pMessage);
    cJSON *json, *jtype, *data;
    char * pReply = nullptr;
    json = cJSON_Parse(pMessage);
    if(json != nullptr){
        if((jtype = cJSON_GetObjectItem(json, "type")) != nullptr && jtype->type == cJSON_String){
            if (strcmp(jtype->valuestring, "config_info") == 0) {
                pReply = pKernel->process_message_configinfo();
            } else if (strcmp(jtype->valuestring, "devices") == 0) {
                pReply = pKernel->process_message_devices();
            } else if (strcmp(jtype->valuestring, "irsend") == 0) {
                if((data=cJSON_GetObjectItem(json, "data")) != nullptr && data->type == cJSON_Object){
                    pReply = pKernel->process_message_sendir(data);
                }
            } else if (strcmp(jtype->valuestring, "irrecv") == 0) {
                if((data=cJSON_GetObjectItem(json, "data")) != nullptr && data->type == cJSON_Object){
                    pReply = pKernel->process_message_recvir(data);
                }
            } else if (strcmp(jtype->valuestring, "bindthrld") == 0) {
                if((data=cJSON_GetObjectItem(json, "data")) != nullptr && data->type == cJSON_Object){
                    pReply = pKernel->process_message_bind_threshold(data);
                }
            } else if (strcmp(jtype->valuestring, "bind") == 0) {
                pReply = pKernel->process_message_bind();
            } else if (strcmp(jtype->valuestring, "removebind") == 0) {
                if((data=cJSON_GetObjectItem(json, "data")) != nullptr && data->type == cJSON_Object){
                    pReply = pKernel->process_message_removebind(data);
                }
            } else if (strcmp(jtype->valuestring, "heartbeat") == 0) {
                pReply = pMessage;
            } else if (strcmp(jtype->valuestring, "subscribe") == 0){
                pReply = pKernel->process_message_subscribe(pClient);
            } else if (strcmp(jtype->valuestring, "setinterval") == 0){
                if((data=cJSON_GetObjectItem(json, "data")) != nullptr && data->type == cJSON_Object){
                    pReply = pKernel->process_message_setinterval(data);
                }
            }
        }
        cJSON_Delete(json);
    }
    if(pReply != nullptr){
        int len = strlen(pReply);
        pKernel->m_server.SendMessage(pClient, pReply, len);
        if(pReply != pMessage && pReply != bind_mode_successful && pReply != bind_mode_failed){
            free(pReply);
        }
    }
}

void Kernel::Restart(){
    xTaskCreate(Kernel::task_Restart, "task_restart", 1024, NULL, tskIDLE_PRIORITY, NULL);
}

uint8_t * Kernel::ReadStreamFromASC(char * asc){
    if(asc == nullptr){
        return nullptr;
    }
    int len = strlen(asc);
    if(len % 2 != 0){
        return nullptr;
    }
    len /= 2;
    uint8_t * stream = (uint8_t *)malloc(len);
    for(int n = 0; n < len; n++){
        uint8_t high, low;
        high = asc[2 * n];
        low = asc[2 * n + 1];
        if(high>= '0' && high <= '9'){
            high -= 0x30;
        }
        else if(high >= 'A' && high <='F'){
            high -= 0x37;
        }
        else if(high >= 'a' && high <='f'){
            high -= 0x57;
        }
        else{
            free(stream);
            return nullptr;
        }
        if(low >= '0' && low <= '9'){
            low -= 0x30;
        }
        else if(low >= 'A' && low <='F'){
            low -= 0x37;
        }
        else if(low >= 'a' && low <='f'){
            low -= 0x57;
        }
        else{
            free(stream);
            return nullptr;
        }
        stream[n] = (high <<4) | low;
    }
    return stream;
}

char * Kernel::process_message_configinfo(){
    const char * version = mdns_get_version();
    const char * serialno = mdns_get_serial();
    int len = strlen(template_config_info);
    len += strlen(version);
    len += strlen(serialno);
    char * pMessage = (char *)malloc(len + 1);
    sprintf(pMessage, template_config_info, version, serialno, m_nUpdateInterval);
    return pMessage;
}

char * Kernel::process_message_devices(){
    int count = 0;
    m_lock.Lock();
    for(auto it: m_mapInfos){
        if(it.second->m_update <= 1 && it.second->m_update >= 0){
            count ++;
        }
    }
    char * devices = (char *)malloc(count *(strlen(template_device) + 220) + 5);
    devices[0] = '[';
    count = 1;
    for(auto it: m_mapInfos){
        if(it.second->m_update <= 1){
            if(it.second->m_update <= 1 && it.second->m_update >= 0){
                int single = sprintf(devices + count, template_device, 
                    it.second->m_addr.toString().c_str(), it.second->serialize_to_json());
                count += single;
            }
        }
    }
    m_lock.Unlock();
    if(count > 1){
        count--;
    }
    devices[count] = ']';
    devices[count + 1] = 0;
    char * json = (char *)malloc(strlen(template_devices) + strlen(devices));
    sprintf(json, template_devices, devices);
    free(devices);
    return json;
}

char * Kernel::process_message_sendir(cJSON * data){
    cJSON * key, * ircode,* device;
    if((key = cJSON_GetObjectItem(data, "key")) != nullptr && key->type == cJSON_String &&
            (device=cJSON_GetObjectItem(data, "device")) != nullptr && device->type == cJSON_String){
        BLEAddress addr(device->valuestring);
        bool bFound = false;
        m_lock.Lock();
        if(m_mapInfos.find(addr) != m_mapInfos.end()){
            bFound = true;
        }
        m_lock.Unlock();
        if(bFound){
            char * Key = key->valuestring;
            char * IRCode = nullptr;
            uint8_t key_len = strlen(Key) / 2;
            uint8_t ircode_len = 0;
            if((ircode = cJSON_GetObjectItem(data, "ircode")) != nullptr && ircode->type == cJSON_String){
                IRCode = ircode->valuestring;
                ircode_len = strlen(IRCode) / 2;
            }
            uint8_t * key_stream = ReadStreamFromASC(Key);
            uint8_t * ircode_stream = ReadStreamFromASC(IRCode);
            if(key_stream != nullptr && ((ircode_stream == nullptr && ircode_len == 0) || ircode_stream != nullptr)){
                if(m_pRemoterInterface->SendIRCode(addr, key_stream, key_len, ircode_stream, ircode_len)){
                    led_set_mode(LED_MODE_FAST_BLINK, 1);
                }
            }
            else{
                ESP_LOGW(LOG_TAG, "Invalid IR code");
            }
            if(key_stream != nullptr){
                free(key_stream);
            }
            if(ircode_stream != nullptr){
                free(ircode_stream);
            }
        }
        m_lock.Unlock();
    }
    return nullptr;
}

char * Kernel::process_message_recvir(cJSON * data){
    cJSON * device;
    if((device=cJSON_GetObjectItem(data, "device")) != nullptr && device->type == cJSON_String){
        BLEAddress addr(device->valuestring);
        bool bFound = false;
        m_lock.Lock();
        if(m_mapInfos.find(addr) != m_mapInfos.end()){
            bFound = true;
        }
        m_lock.Unlock();
        if(bFound){
            led_set_mode(LED_MODE_FAST_BLINK, 0);
            m_pRemoterInterface->ReceiveIRCode(addr);
            if(wifi_connected()){
                led_set_mode(LED_MODE_ON, 0);
            }
            else{
                led_set_mode(LED_MODE_OFF, 0);
            }
        }
    }
    return nullptr;
}

char * Kernel::process_message_bind_threshold(cJSON * data){
    cJSON * threshold;
    if((threshold=cJSON_GetObjectItem(data, "threshold")) != nullptr && threshold->type == cJSON_Number){
        if(threshold->valueint >= -60 && threshold->valueint <= -15){
            nvs_write_bindthrld((int8_t)threshold->valueint);
            GATTDevice::SetBindThreshold(threshold->valueint);
            ESP_LOGI(LOG_TAG, "Bind RSSI threshold set to %d", threshold->valueint);
        }
    }
    return nullptr;
}

char *  Kernel::process_message_bind(){
    char * result = (char *)bind_mode_failed;
    if(m_mode == KernelMode::Normal){
        if(GATTDevice::StartScan()){
            m_mode = KernelMode::ReadyBind;
            result = (char *)bind_mode_successful;
        }
    }
    return result;
}

char * Kernel::process_message_removebind(cJSON * data){
    cJSON * device;
    if((device=cJSON_GetObjectItem(data, "device")) != nullptr && device->type == cJSON_String){
        BLEAddress addr(device->valuestring);
        m_lock.Lock();
        auto it = m_mapInfos.find(addr);
        if(it != m_mapInfos.end()){
            delete it->second;
            m_mapInfos.erase(it);
            SaveAddress();
            int len = strlen(boardcast_template_removebind) + 18;
            char * message = (char *)malloc(len);
            sprintf(message, boardcast_template_removebind, device->valuestring);
            m_server.Boardcast(message, strlen(message));
            free(message);
        }
        m_lock.Unlock();
    }
    return nullptr;
}

char * Kernel::process_message_subscribe(SocketClient * pClient){
    m_server.Subscribe(pClient);
    return nullptr;
}

char * Kernel::process_message_setinterval(cJSON * data){
    cJSON * interval;
    if((interval=cJSON_GetObjectItem(data, "update_interval")) != nullptr && interval->type == cJSON_Number){
        if(interval->valueint >= 1 && interval->valueint <= 30){
            m_nUpdateInterval = interval->valueint;
            nvs_write_interval(m_nUpdateInterval);
            ESP_LOGI(LOG_TAG, "Data update interval set to %d minutes", m_nUpdateInterval);
            int len = strlen(boardcast_template_setinterval) + 2;
            char * message = (char *)malloc(len);
            sprintf(message, boardcast_template_setinterval, m_nUpdateInterval);
            int m = m_server.Boardcast(message, strlen(message));
            ESP_LOGI(LOG_TAG, "boardcast to %d clients", m);
            free(message);
        }
    }
    return nullptr;
}

void Kernel::SaveAddress(){
    uint16_t count = m_mapInfos.size();
    uint8_t * buff = (uint8_t *)malloc(ESP_BD_ADDR_LEN * count);
    count = 0;
    for(auto it: m_mapInfos){
        BLEAddress address = it.first;
        memcpy(buff + count * ESP_BD_ADDR_LEN , address.getNative(), ESP_BD_ADDR_LEN);
        count ++;
    }
    nvs_write_addrs(count, buff, count * ESP_BD_ADDR_LEN);
    ESP_LOGI(LOG_TAG, "Saved %d addres to nvs storage total %d bytes", count, count * ESP_BD_ADDR_LEN);
    free(buff);
}
