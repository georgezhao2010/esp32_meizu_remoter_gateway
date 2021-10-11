/*
 * Created by @georgezhao2010
 * https://github.com/georgezhao2010/esp32_meizu_remoter_gateway/
 * involute@sina.com
 */
#ifndef _GATTREMOTESERVICE_H_
#define _GATTREMOTESERVICE_H_
#include "esp_bt_defs.h"
#include "esp_gattc_api.h"
#include "BLEUUID.h"
#include <map>

class GATTDevice;
class GATTRemoteCharacteristic;

class GATTRemoteService{
public:
    GATTRemoteService(GATTDevice* pDevice, esp_gatt_id_t srvcId, uint16_t startHandle, uint16_t endHandle);
    ~GATTRemoteService();
    GATTRemoteCharacteristic * GetCharacteristic(BLEUUID uuid);
    GATTRemoteCharacteristic * GetCharacteristicByHandle(uint16_t handle);
    inline GATTDevice * GetDevice(){return m_pDevice;}
private:
    void ClearCharacteristics();
    GATTDevice*         m_pDevice;
    esp_gatt_id_t       m_srvcId;
    uint16_t            m_startHandle;
    uint16_t            m_endHandle;
    std::map<BLEUUID, GATTRemoteCharacteristic*> m_mapCharacteristics;
    std::map<uint16_t, GATTRemoteCharacteristic*> m_mapCharacteristicsByHandle;
};

#endif