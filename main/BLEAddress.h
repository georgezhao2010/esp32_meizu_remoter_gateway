/*
 * Created by @georgezhao2010
 * https://github.com/georgezhao2010/esp32_meizu_remoter_gateway/
 * involute@sina.com
 */
#ifndef _BLEADDRESS_H_
#define _BLEADDRESS_H_
#include <esp_gatt_defs.h>
#include <string>


class BLEAddress {
public:
    BLEAddress();
    BLEAddress(esp_bd_addr_t address);
    BLEAddress(const char * stringAddress);
    bool operator==(const BLEAddress& otherAddress) const;
    bool operator!=(const BLEAddress& otherAddress) const;
    bool operator<(const BLEAddress& otherAddress) const;
    bool operator>(const BLEAddress& otherAddress) const;
    inline esp_bd_addr_t* getNative(){return & m_address;}
    std::string toString();
private:
    esp_bd_addr_t m_address;
};
#endif /* _BLEADDRESS_H_ */
