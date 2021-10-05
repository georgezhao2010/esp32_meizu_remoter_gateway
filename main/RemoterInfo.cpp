/*
 * Created by @georgezhao2010
 * https://github.com/georgezhao2010/esp32_meizu_remoter_gateway/
 * involute@sina.com
 */
#include <string.h>
#include "RemoterInfo.h"


const char * json_template = "{\"manufacturer\":\"%s\",\"model\":\"%s\",\"fireware\":\"%s\",\"temperature\":%.2f,\"humidity\":%.2f,\"battery\":%d,\"voltage\":%.1f,\"rssi\":%d}";

MEIZURemoterInfo::MEIZURemoterInfo(){
    m_manufacturer = nullptr;
    m_model = nullptr;
    m_firmware = nullptr;
    m_jsonbuffer = nullptr;
    m_rssi = -127;
    m_update = -1;
}

MEIZURemoterInfo::~MEIZURemoterInfo(){
    if(m_manufacturer != nullptr){
        delete m_manufacturer;
    }
    if(m_model != nullptr){
        delete m_model;
    }
    if(m_firmware != nullptr){
        delete m_firmware;
    }
    if(m_jsonbuffer != nullptr){
        delete m_jsonbuffer;
    }
}

void MEIZURemoterInfo::update_manufacturer(char * manufacturer, uint16_t len){
    if(m_manufacturer != nullptr){
        free(m_manufacturer);
    }
    m_manufacturer = (char *)malloc(len + 1);
    memcpy(m_manufacturer, manufacturer, len);
    m_manufacturer[len] = 0;   
}

void MEIZURemoterInfo::update_model(char * model, uint16_t len){
    if(m_model != nullptr){
        free(m_model);
    }
    m_model = (char *)malloc(len + 1);
    memcpy(m_model, model, len);
    m_model[len] = 0;   
}

void MEIZURemoterInfo::update_firmware(char * firmware, uint16_t len){
    if(m_firmware != nullptr){
        free(m_firmware);
    }
    m_firmware = (char *)malloc(len + 1);
    memcpy(m_firmware, firmware, len);
    m_firmware[len] = 0;   
}

void MEIZURemoterInfo::update_voltage(uint8_t voltage){
    if(voltage >= 30){
        m_battery = 100;
    }
    else {
        switch(voltage){
            case 29:
                m_battery = 90;
                break;
            case 28:
                m_battery = 80;
                break;
            case 27:
                m_battery = 60;
                break;
            case 26:
                m_battery = 30;
                break;
            default:
                m_battery = 10;
        }
    }
    m_voltage = voltage / 10.0f;
}

char * MEIZURemoterInfo::serialize_to_json(){
    if(m_jsonbuffer == nullptr){
        int len = strlen(json_template) + strlen(m_manufacturer) + strlen(m_model) + strlen(m_firmware) + 6 + 6 + 3 + 3 + 3;
        m_jsonbuffer = (char *)malloc(len);
    }
    sprintf(m_jsonbuffer, json_template, m_manufacturer, m_model, m_firmware, m_temperature, m_humidity, m_battery, m_voltage, m_rssi);
    return m_jsonbuffer;
}