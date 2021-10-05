/*
 * Created by @georgezhao2010
 * https://github.com/georgezhao2010/esp32_meizu_remoter_gateway/
 * involute@sina.com
 */
#ifndef _REMOTER_INFO_H_
#define _REMOTER_INFO_H_
#include "BLEAddress.h"

class MEIZURemoterInfo{
public:
    MEIZURemoterInfo();
    ~MEIZURemoterInfo();
    void update_manufacturer(char * manufacturer, uint16_t len);
    void update_model(char * model, uint16_t len);
    void update_firmware(char * firmware, uint16_t len);
    inline void update_temperature(float temperature){m_temperature = temperature;}
    inline void update_humidity(float humidity){m_humidity = humidity;}
    void update_voltage(uint8_t voltage);
    bool has_manufacturer(){return m_manufacturer != nullptr;}
    bool has_model(){return m_model != nullptr;}
    bool has_firmware(){return m_firmware != nullptr;}
    char * serialize_to_json();
public:
    int m_update;
    BLEAddress m_addr;
    int8_t m_rssi;
private:
    char * m_manufacturer;
    char * m_model;
    char * m_firmware;
    float m_temperature;
    float m_humidity;
    float m_voltage;
    uint16_t m_battery;
    char * m_jsonbuffer;
};
#endif