/*
 * Created by @georgezhao2010
 * https://github.com/georgezhao2010/esp32_meizu_remoter_gateway/
 * involute@sina.com
 */
#include "GATTRemoteService.h"
#include "GATTRemoteCharacteristic.h"
#include "GATTDevice.h"
#include "esp_log.h"

GATTRemoteService::GATTRemoteService(GATTDevice* pDevice, esp_gatt_id_t srvcId, uint16_t startHandle, uint16_t endHandle){
    m_pDevice = pDevice;
	m_srvcId = srvcId;
	m_startHandle = startHandle;
	m_endHandle = endHandle;
	uint16_t offset = 0;
	esp_gattc_char_elem_t result;
	while (true){
		uint16_t count = 1;
		esp_gatt_status_t status = ::esp_ble_gattc_get_all_char(
			m_pDevice->GetInterface(),
			m_pDevice->GetConnID(),
			m_startHandle,
			m_endHandle,
			&result,
			&count,
			offset
		);
		if (status == ESP_GATT_INVALID_OFFSET || status != ESP_GATT_OK || count == 0) {
			break;
		}
		BLEUUID uuid = BLEUUID(result.uuid);
		if(pDevice->IsInterestedCharacteristic(uuid)){
			GATTRemoteCharacteristic * pCharacteristic = new GATTRemoteCharacteristic(this, result.char_handle, result.properties);
			m_mapCharacteristics.insert(std::pair<BLEUUID, GATTRemoteCharacteristic*>(uuid, pCharacteristic));
			m_mapCharacteristicsByHandle.insert(std::pair<uint16_t, GATTRemoteCharacteristic*>(result.char_handle, pCharacteristic));
		}
		offset++;
	}
}

GATTRemoteService::~GATTRemoteService(){
	ClearCharacteristics();
}

void GATTRemoteService::ClearCharacteristics(){
    for (auto &it : m_mapCharacteristics){
	   delete it.second;
	}
	m_mapCharacteristics.clear();
	m_mapCharacteristicsByHandle.clear();
}

GATTRemoteCharacteristic * GATTRemoteService::GetCharacteristic(BLEUUID uuid){
	GATTRemoteCharacteristic * ret = nullptr;
    if(m_mapCharacteristics.find(uuid) != m_mapCharacteristics.end()){
        ret = m_mapCharacteristics.find(uuid)->second;
    }
    return ret;
}

GATTRemoteCharacteristic * GATTRemoteService::GetCharacteristicByHandle(uint16_t handle){
	GATTRemoteCharacteristic * ret = nullptr;
    if(m_mapCharacteristicsByHandle.find(handle) != m_mapCharacteristicsByHandle.end()){
        ret = m_mapCharacteristicsByHandle.find(handle)->second;
    }
    return ret;
}