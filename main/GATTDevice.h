/*
 * Created by @georgezhao2010
 * https://github.com/georgezhao2010/esp32_meizu_remoter_gateway/
 * involute@sina.com
 */
#ifndef _GATTDEVICE_H_
#define _GATTDEVICE_H_
#include <esp_gap_ble_api.h>
#include <esp_gattc_api.h>
#include "BLEUUID.h"
#include "BLEAddress.h"
#include "SemaphoreLock.h"
#include <map>

enum class GATTConnectionState{
    Idle = 0,
    Connecting = 1,
    Connected = 2,
    Failed =3
};

typedef void (*on_bind)(void *, bool);
typedef void (*on_device_found)(void *, esp_bd_addr_t *);
typedef void (*on_rssi_read)(void *, esp_bd_addr_t *, int8_t);

class GATTRemoteCharacteristic;
class GATTRemoteService;
class GATTDevice{
public:
    GATTDevice();
    ~GATTDevice();
    bool Connect(BLEAddress addr);
    void Close();
    static bool Init(void * pCaller, on_bind onBind, on_device_found OnDeviceFound, on_rssi_read OnRSSIRead);
    static void SetBindThreshold(int threshold);
    static bool StartScan();
    static bool StopScan();
    bool Read(GATTRemoteCharacteristic * pCharacteristic);
    bool Write(GATTRemoteCharacteristic * pCharacteristic, uint8_t* data, uint16_t length, bool response = false);
    virtual void OnNotify(GATTRemoteCharacteristic * pCharacteristic, uint8_t * data, uint16_t length) = 0;
    GATTRemoteService * GetService(const char * uuid);
    GATTRemoteService * GetService(BLEUUID uuid);
    inline GATTConnectionState ConnectingState(){return m_connectingState;}
    inline esp_gatt_if_t GetInterface(){return m_gattc_if;}
    inline uint16_t GetConnID(){return m_connid;}
    inline BLEAddress & GetBLEAddress(){return m_addr;}
    inline void AddInterestedServices(BLEUUID uuid){m_mapInterestedServices.insert(std::pair<BLEUUID, bool>(uuid, true));}
    inline void AddInterestedCharacteristic(BLEUUID uuid){m_mapInterestedCharacteristic.insert(std::pair<BLEUUID, bool>(uuid, true));}
    inline bool IsInterestedServices(BLEUUID uuid){return m_mapInterestedServices.empty() || m_mapInterestedServices.find(uuid) != m_mapInterestedServices.end();}
    inline bool IsInterestedCharacteristic(BLEUUID uuid){return m_mapInterestedCharacteristic.empty() || m_mapInterestedCharacteristic.find(uuid) != m_mapInterestedCharacteristic.end();}
private:
    static void GAPEventHandler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
    static void GATTEventHandler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);
    void HandleEvent(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);
    void ClearServices();
protected:
    SemaphoreLock * m_lock;
    uint16_t m_appid;
    uint16_t m_connid;
    esp_gatt_if_t m_gattc_if;
    GATTConnectionState m_connectingState;
    std::map<BLEUUID, GATTRemoteService*> m_mapServices;
    std::map<BLEUUID, bool> m_mapInterestedServices;
    std::map<BLEUUID, bool> m_mapInterestedCharacteristic;
    BLEAddress m_addr;
    int8_t m_rssi;
    uint8_t m_readbuff[256];
    uint16_t m_readlen;
};

#endif