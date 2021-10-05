/*
 * Created by @georgezhao2010
 * https://github.com/georgezhao2010/esp32_meizu_remoter_gateway/
 * involute@sina.com
 */
#ifndef _GATTREMOTECHARACTERISTIC_H_
#define _GATTREMOTECHARACTERISTIC_H_
#include "BLEUUID.h"
#include "esp_gattc_api.h"
#include <map>

class GATTRemoteService;
class GATTRemoteDescriptor;

class GATTRemoteCharacteristic{
public:
    GATTRemoteCharacteristic(GATTRemoteService *pService, uint16_t handle, esp_gatt_char_prop_t charProp);
    ~GATTRemoteCharacteristic();
    inline bool FeatureSupports(uint8_t feature){return ((m_charProp & feature) == feature);}
    bool Write(uint8_t* data, uint16_t length, bool response = false);
    bool Read();
    bool RegNotify();
    bool UnregNotify();
    inline GATTRemoteService * GetService(){return m_pService;}
private:
    GATTRemoteService * m_pService;
    uint16_t m_handle;
    esp_gatt_char_prop_t m_charProp;
};

#endif