/*
 * Created by @georgezhao2010
 * https://github.com/georgezhao2010/esp32_meizu_remoter_gateway/
 * involute@sina.com
 */
#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_gatt_common_api.h>
#include <esp_log.h>
#include <string.h>
#include "GATTDevice.h"
#include "GATTRemoteService.h"
#include "GATTRemoteCharacteristic.h"

#define LOG_TAG "GATTDevice"

uint16_t g_appid = 0;
std::map<uint16_t, GATTDevice*> g_appidmap;
std::map<esp_gatt_if_t, GATTDevice*> g_ifmap;

on_bind g_bind = nullptr;
on_device_found g_device_found = nullptr;
on_rssi_read g_rssi_read = nullptr;
void * g_caller = nullptr;
bool g_scanning = false;
int g_scan_threshold = CONFIG_BLE_RSSI_HIGH_THRLD;

GATTDevice::GATTDevice(){
    m_lock = new SemaphoreLock(true);
    m_gattc_if = ESP_GATT_IF_NONE;
    m_appid = g_appid++;
    m_connectingState = GATTConnectionState::Idle;
    g_appidmap.insert(std::pair<uint16_t, GATTDevice*>(m_appid, this));
    esp_err_t ret = esp_ble_gattc_app_register(m_appid);
    if (ret != ESP_OK){
        ESP_LOGE("GATTDevice::GATTDevice", "Gattc app register failed, error code = %x", ret);
        return;
    }
}

GATTDevice::~GATTDevice(){
    ClearServices();
    esp_ble_gattc_app_unregister(m_appid);
    if(g_appidmap.find(m_appid) != g_appidmap.end())
		g_appidmap.erase(m_appid);
}

bool GATTDevice::Connect(BLEAddress addr){
    m_connectingState = GATTConnectionState::Connecting;
    m_addr = addr;
    esp_err_t ret = esp_ble_gattc_open(m_gattc_if, *m_addr.getNative(), BLE_ADDR_TYPE_PUBLIC, true);
    if(ret != ESP_OK){
        m_connectingState = GATTConnectionState::Idle;
        ESP_LOGE("GATTDevice::Open", "Gattc open failed, error code = %x", ret);
        return false;
    }
    else{
        m_lock->Lock();
    }
    if(m_connectingState == GATTConnectionState::Connected && m_mapServices.size() == 0){
        ret = esp_ble_gattc_search_service(m_gattc_if, m_connid, NULL);
        m_lock->Lock();
    }
    return m_connectingState == GATTConnectionState::Connected;
}

void GATTDevice::Close(){
    esp_ble_gattc_close(m_gattc_if, m_connid);
}

bool GATTDevice::Init(void * pCaller, on_bind onBind, on_device_found OnDeviceFound, on_rssi_read OnRSSIRead){
    g_caller = pCaller;
    g_bind = onBind;
    g_device_found = OnDeviceFound;
    g_rssi_read = OnRSSIRead;
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    esp_err_t ret;
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(LOG_TAG, "Failed to bluetooth controller initialization: %s", esp_err_to_name(ret));
        return false;
    }
    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(LOG_TAG, "Failed to enable bluetooth controller: %s\n", esp_err_to_name(ret));
        return false;
    }
    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(LOG_TAG, "Failed to bluetooth initializatio: %s\n", esp_err_to_name(ret));
        return false;
    }
    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(LOG_TAG, "Failed to enable bluetooth: %s\n", esp_err_to_name(ret));
        return false;
    }
    ret = esp_ble_gap_register_callback(GATTDevice::GAPEventHandler);
    if (ret){
        ESP_LOGE(LOG_TAG, "Failed to GAP register: %s", esp_err_to_name(ret));
        return false;
    }
    ret = esp_ble_gattc_register_callback(GATTDevice::GATTEventHandler);
    if(ret){
        ESP_LOGE(LOG_TAG, "Failed to GATTC register: %s", esp_err_to_name(ret));
        return false;
    }
    ret = esp_ble_gatt_set_local_mtu(500);
    if (ret){
        ESP_LOGE(LOG_TAG, "Failed to set local MTU:%s", esp_err_to_name(ret));
        return false;
    }
    return true;
}

void GATTDevice::GAPEventHandler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t * param){
    switch (event){
        case ESP_GAP_BLE_READ_RSSI_COMPLETE_EVT:
            if (param->read_rssi_cmpl.status != ESP_BT_STATUS_SUCCESS){
                ESP_LOGE(LOG_TAG, "Read rssi failed, error status = %x", param->read_rssi_cmpl.status);
                break;
            }
            if(g_rssi_read != nullptr){
                g_rssi_read(g_caller, &param->read_rssi_cmpl.remote_addr, param->read_rssi_cmpl.rssi);
            }
            break; 
        case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
            break;
        case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
            if (param->scan_start_cmpl.status != ESP_BT_STATUS_SUCCESS){
                ESP_LOGE(LOG_TAG, "Scan start failed, error status = %x", param->scan_start_cmpl.status);
                if(g_bind != nullptr){
                    g_bind(g_caller, false);
                }
                break;
            }
            ESP_LOGI(LOG_TAG, "BLE devices scanning");
            if(g_bind != nullptr){
                g_bind(g_caller, true);
            }
            break;
        case ESP_GAP_BLE_SCAN_RESULT_EVT:{
                esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
                switch (scan_result->scan_rst.search_evt){
                    case ESP_GAP_SEARCH_INQ_RES_EVT:{
                            uint8_t *adv_name = NULL;
                            uint8_t adv_name_len = 0;
                            adv_name = esp_ble_resolve_adv_data(scan_result->scan_rst.ble_adv,ESP_BLE_AD_TYPE_NAME_CMPL, &adv_name_len);
                            if(scan_result->scan_rst.rssi >= g_scan_threshold)
                            {
                                char * pName = (char *)malloc(adv_name_len + 1);
                                memcpy(pName, adv_name, adv_name_len);
                                pName[adv_name_len] = 0;
                                ESP_LOGI(LOG_TAG, "Nearly device found - %s", 
                                    pName[0] == 0? "Noname device": pName);
                                if (scan_result->scan_rst.scan_rsp_len == 10 && memcmp(&scan_result->scan_rst.ble_adv[scan_result->scan_rst.adv_data_len + 4], "RW-BLE", 6) == 0){
                                    if(g_device_found!=nullptr){
                                        g_device_found(g_caller, &scan_result->scan_rst.bda);
                                    }
                                }
                                free(pName);
                            }
                        }
                        break;
                    case ESP_GAP_SEARCH_INQ_CMPL_EVT:
                        g_scanning = false;
                        ESP_LOGI(LOG_TAG, "No new device found, stop scan");
                        if(g_bind != nullptr){
                            g_bind(g_caller, false);
                        }
                        break;
                    default:
                        break;
                }
            }
            break;
        case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
            if (param->scan_stop_cmpl.status == ESP_BT_STATUS_SUCCESS && g_bind != nullptr){
                g_scanning = false;
                g_bind(g_caller, false);
            }
            
            break;
        case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
            if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS){
                ESP_LOGE(LOG_TAG, "Adv stop failed, error status = %x", param->adv_stop_cmpl.status);
                break;
            }
            break;
        default:
            break;
    }
}

void GATTDevice::SetBindThreshold(int threshold){
    g_scan_threshold = threshold;
}

bool GATTDevice::StartScan(){
    if(g_scanning == false){
        uint32_t duration = CONFIG_BLE_BIND_DURATION;
        esp_err_t ret = esp_ble_gap_start_scanning(duration);
        if(ret != ESP_OK){
            ESP_LOGI(LOG_TAG, "Failed to start binding mode: %s", esp_err_to_name(ret));
            return false;
        }
        ESP_LOGI(LOG_TAG, "Start binding mode");
        g_scanning = true;
        return true;
    }
    return false;
}

bool GATTDevice::StopScan(){
    if(g_scanning == true){
        esp_ble_gap_stop_scanning();
        return true;
    }
    return false;
}

void GATTDevice::GATTEventHandler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param){
    if (event == ESP_GATTC_REG_EVT){
        if (param->reg.status == ESP_GATT_OK){
            if(g_appidmap.find(param->reg.app_id) != g_appidmap.end()){
                GATTDevice * pThis = g_appidmap.find(param->reg.app_id)->second;
                pThis->m_gattc_if = gattc_if;
                g_ifmap.insert(std::pair<esp_gatt_if_t, GATTDevice*>(gattc_if, pThis));
            }
        } 
        else{
            ESP_LOGI(LOG_TAG, "Failed to reg app, app_id %04x, status %d",
                    param->reg.app_id,
                    param->reg.status);
        }
    }
    else if(gattc_if != ESP_GATT_IF_NONE){
        if(g_ifmap.find(gattc_if) != g_ifmap.end())
            g_ifmap.find(gattc_if)->second->HandleEvent(event, gattc_if, param);
    }
}

void GATTDevice::HandleEvent(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param){
    if (m_gattc_if == ESP_GATT_IF_NONE && event != ESP_GATTC_REG_EVT){
		return;
	}
    
    switch(event){
        case ESP_GATTC_OPEN_EVT:
            // 2 When GATT virtual connection is set up, the event comes
            if(param->open.status == ESP_GATT_OK){
                m_connid = param->open.conn_id;
                esp_ble_gap_read_rssi(*m_addr.getNative());
            }
            else
            {
                m_connectingState = GATTConnectionState::Failed;
                m_lock->Unlock();
            }
            break;
        case ESP_GATTC_CONNECT_EVT: 
            // 40 When the ble physical connection is set up, the event comes
            break;
        case ESP_GATTC_DIS_SRVC_CMPL_EVT: 
            // 46 When the ble discover service complete, the event comes
            // 40 2 46
            m_connectingState = GATTConnectionState::Connected;
            m_lock->Unlock();
            break;
        case ESP_GATTC_DISCONNECT_EVT:
            // 41 When the ble physical connection disconnected, the event comes
            break;
        case ESP_GATTC_SEARCH_CMPL_EVT:
            // 6 When GATT service discovery is completed, the event comes
            //ESP_LOGW(LOG_TAG, "ESP_GATTC_SEARCH_CMPL_EVT Unlock");
            m_lock->Unlock();
            break;
        case ESP_GATTC_SEARCH_RES_EVT:{
            // 7 When GATT service discovery result is got, the event comes
                BLEUUID uuid = BLEUUID(param->search_res.srvc_id.uuid);
                if(IsInterestedServices(uuid)){
                    GATTRemoteService* pRemoteService = new GATTRemoteService(
                        this,
                        param->search_res.srvc_id,
                        param->search_res.start_handle,
                        param->search_res.end_handle
                    );
                    m_mapServices.insert(std::pair<BLEUUID, GATTRemoteService*>(uuid, pRemoteService));
                }
            }
            break;
        case ESP_GATTC_READ_CHAR_EVT:
            if(param->read.status == ESP_GATT_OK){
                m_readlen = param->read.value_len;
                memcpy(m_readbuff, param->read.value, m_readlen);
                m_readbuff[m_readlen] = 0;
            }
            else{
                m_readlen = 0;
            }
            m_lock->Unlock();
            break;
        case ESP_GATTC_WRITE_CHAR_EVT:
            m_lock->Unlock();
            break;
        case ESP_GATTC_NOTIFY_EVT:
            for (auto &it : m_mapServices){
                GATTRemoteCharacteristic *pCharacteristic = it.second->GetCharacteristicByHandle(param->notify.handle);
                if(pCharacteristic != nullptr){
                    OnNotify(pCharacteristic, param->notify.value, param->notify.value_len);
                }
            }
            break;
        default:
            break;
    }
}

void GATTDevice::ClearServices(){
    for (auto &it : m_mapServices) {
	   delete it.second;
	}
	m_mapServices.clear();
}

GATTRemoteService * GATTDevice::GetService(const char * uuid){
    return GetService(BLEUUID(uuid));
}

GATTRemoteService * GATTDevice::GetService(BLEUUID uuid){
    GATTRemoteService * ret = nullptr;
    if(m_mapServices.find(uuid) != m_mapServices.end()){
        ret = m_mapServices.find(uuid)->second;
    }
    return ret;
}

bool GATTDevice::Read(GATTRemoteCharacteristic * pCharacteristic){
    bool ret = false;
    if(pCharacteristic != nullptr){
        ret = pCharacteristic->Read();
        if(ret){
            m_lock->Lock();
            if(m_readlen == 0){
                ret = false;
            }
        }
    }
    return ret;
}

bool GATTDevice::Write(GATTRemoteCharacteristic * pCharacteristic, uint8_t* data, uint16_t length, bool response){
    bool ret = false;
    if(pCharacteristic != nullptr){
        ret = pCharacteristic->Write(data, length, response);
        if(ret){
            m_lock->Lock();
        }
    }
    return ret;
}

