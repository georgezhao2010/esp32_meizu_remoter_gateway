/*
 * Created by @georgezhao2010
 * https://github.com/georgezhao2010/esp32_meizu_remoter_gateway/
 * involute@sina.com
 */
#ifndef _BLEUUID_H_
#define _BLEUUID_H_
#include <esp_gatt_defs.h>
#include <string>

class BLEUUID{
public:
	BLEUUID();
	BLEUUID(esp_bt_uuid_t uuid);
	BLEUUID(const char * uuid);
	BLEUUID(uint16_t uuid);
	BLEUUID(uint32_t uuid);
	bool operator==(const BLEUUID& otherUUID) const;
	bool operator!=(const BLEUUID& otherUUID) const;
	bool operator<(const BLEUUID& otherUUID) const;
	bool operator>(const BLEUUID& otherUUID) const;
	esp_bt_uuid_t* getNative(){return &m_uuid;}
	std::string toString();
private:
	esp_bt_uuid_t m_uuid;
}; 
#endif /* _BLEUUID_H_ */
