/*
 * Created by @georgezhao2010
 * https://github.com/georgezhao2010/esp32_meizu_remoter_gateway/
 * involute@sina.com
 */
#include "BLEAddress.h"
#include "esp_log.h"
#include "string.h"

BLEAddress::BLEAddress(){

}

BLEAddress::BLEAddress(esp_bd_addr_t address){
	memcpy(m_address, address, ESP_BD_ADDR_LEN);
}


BLEAddress::BLEAddress(const char * stringAddress){
	if (strlen(stringAddress) != 17){
		ESP_LOGE("BLEAddress", "Invalid BLEAddress format");
	}
	int data[6];
	sscanf(stringAddress, "%x:%x:%x:%x:%x:%x", &data[0], &data[1], &data[2], &data[3], &data[4], &data[5]);
	m_address[0] = (uint8_t) data[0];
	m_address[1] = (uint8_t) data[1];
	m_address[2] = (uint8_t) data[2];
	m_address[3] = (uint8_t) data[3];
	m_address[4] = (uint8_t) data[4];
	m_address[5] = (uint8_t) data[5];
}

bool BLEAddress::operator==(const BLEAddress& otherAddress) const{
	return memcmp(otherAddress.m_address, m_address, ESP_BD_ADDR_LEN) == 0;
}

bool BLEAddress::operator!=(const BLEAddress& otherAddress) const{
  	return memcmp(otherAddress.m_address, m_address, ESP_BD_ADDR_LEN) != 0;
}

bool BLEAddress::operator<(const BLEAddress& otherAddress) const{
	return memcmp(otherAddress.m_address, m_address, ESP_BD_ADDR_LEN) < 0;
}

bool BLEAddress::operator>(const BLEAddress& otherAddress)const {
	return memcmp(otherAddress.m_address, m_address, ESP_BD_ADDR_LEN) > 0;
}

std::string BLEAddress::toString(){
	char str[18];
	sprintf(str, "%02x:%02x:%02x:%02x:%02x:%02x", m_address[0], m_address[1], m_address[2], m_address[3], m_address[4], m_address[5]);
	return std::string(str);
}