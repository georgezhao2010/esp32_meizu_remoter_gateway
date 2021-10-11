/*
 * Created by @georgezhao2010
 * https://github.com/georgezhao2010/esp32_meizu_remoter_gateway/
 * involute@sina.com
 */
#include "BLEUUID.h"
#include "esp_log.h"
#include "string.h"

BLEUUID::BLEUUID(){
    m_uuid.len = 0;
}

BLEUUID::BLEUUID(const char * uuid){
    size_t uuidlen  = strlen(uuid);
    if (uuidlen == 4) {
        m_uuid.len = ESP_UUID_LEN_16;
        m_uuid.uuid.uuid16 = 0;
        for(int i=0;i<uuidlen;){
            uint8_t MSB = uuid[i];
            uint8_t LSB = uuid[i+1];
            
            if(MSB > '9') MSB -= 7;
            if(LSB > '9') LSB -= 7;
            m_uuid.uuid.uuid16 += (((MSB&0x0F) <<4) | (LSB & 0x0F))<<(2-i)*4;
            i+=2;    
        }
    }
    else if (uuidlen == 8){
        m_uuid.len = ESP_UUID_LEN_32;
        m_uuid.uuid.uuid32 = 0;
        for(int i=0;i<uuidlen;){
            uint8_t MSB = uuid[i];
            uint8_t LSB = uuid[i+1];
            
            if(MSB > '9') MSB -= 7; 
            if(LSB > '9') LSB -= 7;
            m_uuid.uuid.uuid32 += (((MSB&0x0F) <<4) | (LSB & 0x0F))<<(6-i)*4;
            i+=2;
        }        
    }
    else if (uuidlen == 16) {
        m_uuid.len = ESP_UUID_LEN_128;
        memcpy(m_uuid.uuid.uuid128, uuid, 16);
    }
    else if (uuidlen == 36) {
        m_uuid.len = ESP_UUID_LEN_128;
        int n = 0;
        for(int i=0;i<uuidlen;){
            if(uuid[i] == '-')
                i++;
            uint8_t MSB = uuid[i];
            uint8_t LSB = uuid[i+1];
            
            if(MSB > '9') MSB -= 7; 
            if(LSB > '9') LSB -= 7;
            m_uuid.uuid.uuid128[15-n++] = ((MSB&0x0F) <<4) | (LSB & 0x0F);
            i+=2;    
        }
    }
    else{
        ESP_LOGE("BLEUUID", "UUID value not 4, 8, 16 or 36 bytes");
    }
}

BLEUUID::BLEUUID(uint16_t uuid) {
    m_uuid.len         = ESP_UUID_LEN_16;
    m_uuid.uuid.uuid16 = uuid;
}

BLEUUID::BLEUUID(uint32_t uuid) {
    m_uuid.len         = ESP_UUID_LEN_32;
    m_uuid.uuid.uuid32 = uuid;
}

BLEUUID::BLEUUID(esp_bt_uuid_t uuid){
    m_uuid = uuid;
}

bool BLEUUID::operator==(const BLEUUID& otherUUID) const{
    bool ret = false;
    if(m_uuid.len == otherUUID.m_uuid.len){
        switch(m_uuid.len){
            case ESP_UUID_LEN_16:
                ret = (m_uuid.uuid.uuid16 == otherUUID.m_uuid.uuid.uuid16);
                break;
            case ESP_UUID_LEN_32:
                ret = (m_uuid.uuid.uuid32 == otherUUID.m_uuid.uuid.uuid32);
                break;
            case ESP_UUID_LEN_128:
                ret = (memcmp(m_uuid.uuid.uuid128, otherUUID.m_uuid.uuid.uuid128, ESP_UUID_LEN_128)==0);
                break;
        }
    }
    return ret;
}

bool BLEUUID::operator!=(const BLEUUID& otherUUID) const {
    return !(*this == otherUUID);
}

bool BLEUUID::operator<(const BLEUUID& otherUUID) const {
    bool ret;
    if(m_uuid.len == otherUUID.m_uuid.len){
        switch(m_uuid.len){
            case ESP_UUID_LEN_16:
                ret = (m_uuid.uuid.uuid16 < otherUUID.m_uuid.uuid.uuid16);
                break;
            case ESP_UUID_LEN_32:
                ret = (m_uuid.uuid.uuid32 < otherUUID.m_uuid.uuid.uuid32);
                break;
            case ESP_UUID_LEN_128:
                ret = (memcmp(m_uuid.uuid.uuid128, otherUUID.m_uuid.uuid.uuid128, ESP_UUID_LEN_128) < 0);
                break;
            default:
                ret = false;
                break;
        }
    }
    else{
        ret = (m_uuid.len < otherUUID.m_uuid.len);
    }
    return ret;
}

bool BLEUUID::operator>(const BLEUUID& otherUUID) const {
    bool ret;
    if(m_uuid.len == otherUUID.m_uuid.len){
        switch(m_uuid.len){
            case ESP_UUID_LEN_16:
                ret = (m_uuid.uuid.uuid16 > otherUUID.m_uuid.uuid.uuid16);
                break;
            case ESP_UUID_LEN_32:
                ret = (m_uuid.uuid.uuid32 > otherUUID.m_uuid.uuid.uuid32);
                break;
            case ESP_UUID_LEN_128:
                ret = (memcmp(m_uuid.uuid.uuid128, otherUUID.m_uuid.uuid.uuid128, ESP_UUID_LEN_128) > 0);
                break;
            default:
                ret = false;
                break;
        }
    }
    else{
        ret = (m_uuid.len > otherUUID.m_uuid.len);
    }
    return ret;
}

std::string BLEUUID::toString(){
    if (m_uuid.len == 0) return "<NULL>"; 

    if (m_uuid.len == ESP_UUID_LEN_16){ // 16bits UUID
        char hex[9];
        snprintf(hex, sizeof(hex), "%08x", m_uuid.uuid.uuid16);
        return std::string(hex) + "-0000-1000-8000-00805f9b34fb";
    } 

    if (m_uuid.len == ESP_UUID_LEN_32){ // 32bits UUID
        char hex[9];
        snprintf(hex, sizeof(hex), "%08x", m_uuid.uuid.uuid32);
        return std::string(hex) + "-0000-1000-8000-00805f9b34fb";
    }
    auto size = 37; // 32 for UUID data, 4 for '-' delimiters and one for a terminator == 37 chars
    char *hex = (char *)malloc(size);
    snprintf(hex, size, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
            m_uuid.uuid.uuid128[15], m_uuid.uuid.uuid128[14],
            m_uuid.uuid.uuid128[13], m_uuid.uuid.uuid128[12],
            m_uuid.uuid.uuid128[11], m_uuid.uuid.uuid128[10],
            m_uuid.uuid.uuid128[9], m_uuid.uuid.uuid128[8],
            m_uuid.uuid.uuid128[7], m_uuid.uuid.uuid128[6],
            m_uuid.uuid.uuid128[5], m_uuid.uuid.uuid128[4],
            m_uuid.uuid.uuid128[3], m_uuid.uuid.uuid128[2],
            m_uuid.uuid.uuid128[1], m_uuid.uuid.uuid128[0]);
    std::string res(hex);
    free(hex);
    return res;
} 