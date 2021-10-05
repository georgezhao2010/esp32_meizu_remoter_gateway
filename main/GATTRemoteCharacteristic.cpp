/*
 * Created by @georgezhao2010
 * https://github.com/georgezhao2010/esp32_meizu_remoter_gateway/
 * involute@sina.com
 */
#include "GATTRemoteCharacteristic.h"
#include "GATTRemoteService.h"
#include "GATTDevice.h"
#include "esp_log.h"

#define LOG_TAG "GATTRemoteCharacteristic"

GATTRemoteCharacteristic::GATTRemoteCharacteristic(
    GATTRemoteService *pService, 
    uint16_t handle, 
    esp_gatt_char_prop_t charProp){
    m_pService = pService;
    m_handle = handle;
    m_charProp = charProp;
}

GATTRemoteCharacteristic::~GATTRemoteCharacteristic(){
	
}


bool GATTRemoteCharacteristic::Write(uint8_t* data, uint16_t length, bool response){
	bool result = false;
	if(FeatureSupports(ESP_GATT_CHAR_PROP_BIT_WRITE) || FeatureSupports(ESP_GATT_CHAR_PROP_BIT_WRITE_NR)){
		esp_err_t ret = esp_ble_gattc_write_char(
			m_pService->GetDevice()->GetInterface(),
			m_pService->GetDevice()->GetConnID(),
			m_handle,
			length,
			data,
			response? ESP_GATT_WRITE_TYPE_RSP: ESP_GATT_WRITE_TYPE_NO_RSP,
			ESP_GATT_AUTH_REQ_NONE
		);
		if (ret == ESP_OK){
			result = true;
		}
		else{
			ESP_LOGE(LOG_TAG, "Failed to write characteristic, reason =  %s", esp_err_to_name(ret));
		}
	}
	else{
		ESP_LOGE(LOG_TAG, "Characteristic does not supports feature write");
	}
    
	return result;
}

bool GATTRemoteCharacteristic::Read(){
	bool result = false;
	if(FeatureSupports(ESP_GATT_CHAR_PROP_BIT_READ)){
		esp_err_t ret = esp_ble_gattc_read_char(
			m_pService->GetDevice()->GetInterface(),
			m_pService->GetDevice()->GetConnID(),
			m_handle,
			ESP_GATT_AUTH_REQ_NONE); 
		if (ret == ESP_OK) {
			result = true;
		}
		else{
			ESP_LOGE(LOG_TAG, "Failed to read characteristic, reason %s", esp_err_to_name(ret));
		}
	}
	else {
		ESP_LOGE(LOG_TAG, "Characteristic does not supports feature read");
	}
	return result;
}

bool GATTRemoteCharacteristic::RegNotify(){
	bool result = false;
	if(FeatureSupports(ESP_GATT_CHAR_PROP_BIT_NOTIFY))
	{
		esp_err_t ret = esp_ble_gattc_register_for_notify(
			m_pService->GetDevice()->GetInterface(),
			*(m_pService->GetDevice()->GetBLEAddress().getNative()),
			m_handle);
		if (ret == ESP_OK) {
			result = true;
		}
		else{
			ESP_LOGE(LOG_TAG, "Failed to register notify, reason = %s", esp_err_to_name(ret));
		}
	}
	else{
		ESP_LOGE(LOG_TAG, "Characteristic does not supports feature notify");
	}
	return result;
}

bool GATTRemoteCharacteristic::UnregNotify(){
	bool result = false;
	if(FeatureSupports(ESP_GATT_CHAR_PROP_BIT_NOTIFY))
	{
		esp_err_t ret = esp_ble_gattc_unregister_for_notify(
			m_pService->GetDevice()->GetInterface(),
			*(m_pService->GetDevice()->GetBLEAddress().getNative()),
			m_handle);
		if (ret == ESP_OK) {
			result = true;
		}
		else{
			ESP_LOGE(LOG_TAG, "Failed to unregister notify, reason = %s", esp_err_to_name(ret));
		}
	}
	else{
		ESP_LOGE(LOG_TAG, "Characteristic does not supports feature notify");
	}
	return result;
}