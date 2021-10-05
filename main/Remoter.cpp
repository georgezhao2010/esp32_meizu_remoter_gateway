/*
 * Created by @georgezhao2010
 * https://github.com/georgezhao2010/esp32_meizu_remoter_gateway/
 * involute@sina.com
 */
#include <esp_log.h>
#include <string.h>
#include "Remoter.h"
#include "GATTRemoteService.h"
#include "GATTRemoteCharacteristic.h"
#include "BLEUUID.h"
#include "utils.h"

#define CMD_PACKET                      (uint8_t)0x00
#define CMD_SEND_IR                     (uint8_t)0x03
#define CMD_SEND_IR_READY               (uint8_t)0x04
#define CMD_RECV_IR                     (uint8_t)0x05
#define CMD_RECV_IR_READY               (uint8_t)0x07
#define CMD_VOLTAGE                     (uint8_t)0x10
#define CMD_TEMP_HUMI                   (uint8_t)0x11

#define MAX_PACKET_PAYLOAD 16

#define LOG_TAG "MEIZURemoter"

static BLEUUID serviceUUID("16F0");
static BLEUUID characteristicUUID("16F2");
static BLEUUID infoUUID("180A");
static BLEUUID manufacturerUUID("2A29");
static BLEUUID modelUUID("2A24");
static BLEUUID firmwareUUID("2A26");

MEIZURemoter::MEIZURemoter(){
    m_sequence = 0;
    AddInterestedServices(serviceUUID);
    AddInterestedServices(infoUUID);
    AddInterestedCharacteristic(characteristicUUID);
    AddInterestedCharacteristic(manufacturerUUID);
    AddInterestedCharacteristic(modelUUID);
    AddInterestedCharacteristic(firmwareUUID);
}

MEIZURemoter::~MEIZURemoter(){

}

bool MEIZURemoter::ReadAllInfo(MEIZURemoterInfo * pInfo){
    m_lockDevice.Lock();
    m_addr = pInfo->m_addr;
    ESP_LOGI(LOG_TAG, "Begin update device at %s", m_addr.toString().c_str());
    if(Connect(m_addr)){
         GATTRemoteService * pService;
        if(!pInfo->has_manufacturer()){
            pService = GetService(infoUUID);
            if(pService != nullptr){
                GATTRemoteCharacteristic * pCh = pService->GetCharacteristic(manufacturerUUID);
                if(pCh != nullptr){
                    if(Read(pCh)){
                        pInfo->update_manufacturer((char *)m_readbuff, m_readlen);
                    }
                }
                pCh = pService->GetCharacteristic(modelUUID);
                if(pCh != nullptr){
                    if(Read(pCh)){
                        pInfo->update_model((char *)m_readbuff, m_readlen);
                    }
                }
                pCh = pService->GetCharacteristic(firmwareUUID);
                if(pCh != nullptr){
                    if(Read(pCh)){
                        pInfo->update_firmware((char *)m_readbuff, m_readlen);
                    }
                }
            }
        }
        pService = GetService(serviceUUID);
        if(pService != nullptr){
            GATTRemoteCharacteristic * pCh = pService->GetCharacteristic(characteristicUUID);
            SendCommand(pCh, ++m_sequence, CMD_TEMP_HUMI);
            Read(pCh);
            if (m_readlen == 8 && m_readbuff[0] == 0x55 && m_readbuff[1] == 0x07 && m_readbuff[3] == CMD_TEMP_HUMI) {
                pInfo->update_temperature(((m_readbuff[5] << 8) + m_readbuff[4]) / 100.0f);
                pInfo->update_humidity(((m_readbuff[7] << 8) + m_readbuff[6]) / 100.0f);
            }
            SendCommand(pCh, ++m_sequence, CMD_VOLTAGE);
            Read(pCh);
            if (m_readlen == 5 && m_readbuff[0] == 0x55 && m_readbuff[1] == 0x04 && m_readbuff[3] == CMD_VOLTAGE) {
                pInfo->update_voltage(m_readbuff[4]);
            }
        }
        pInfo->m_update = 0;
        Close();
        ESP_LOGI(LOG_TAG, "Device at %s was updated", m_addr.toString().c_str());
    }
    else{
        ESP_LOGE(LOG_TAG, "Can not connect to device at %s", m_addr.toString().c_str());
    }
    m_lockDevice.Unlock();
    return pInfo->m_update == 0;
}

bool MEIZURemoter::SendIRCode(BLEAddress addr, uint8_t * key, uint16_t keylen, uint8_t * ircode, uint16_t ircodelen){
    m_lockDevice.Lock();
    bool result = false;
    if(Connect(addr)){
        GATTRemoteService * pService = GetService(serviceUUID);
        if(pService != nullptr){
            GATTRemoteCharacteristic * pCh = pService->GetCharacteristic(characteristicUUID);
            if(SendCommand(pCh, ++m_sequence, CMD_SEND_IR, key, keylen)) {
                Read(pCh);
                if(m_readlen == 5 && m_readbuff[0] == 0x55 && m_readbuff[3] == CMD_SEND_IR_READY && m_readbuff[4] == 0x00)
                {
                    uint8_t packet_count = ircodelen / MAX_PACKET_PAYLOAD + (ircodelen % MAX_PACKET_PAYLOAD > 0? 1: 0);
                    uint8_t streamlen = keylen + 2;
                    uint8_t * stream = (uint8_t *)malloc(streamlen);
                    stream[0] = 0;
                    stream[1] = packet_count + 1;
                    memcpy(stream + 2, key, keylen);
                    SendCommand(pCh, m_sequence, CMD_PACKET, stream, streamlen);
                    free(stream);
                    for(uint16_t n = 0 ; n < packet_count; n++){
                        streamlen = (n == packet_count - 1? ircodelen % MAX_PACKET_PAYLOAD : MAX_PACKET_PAYLOAD) + 1;
                        ESP_LOGI(LOG_TAG, "Stream Len  = %d", streamlen);
                        stream = (uint8_t *)malloc(streamlen);
                        stream[0] = n + 1;
                        memcpy(stream + 1, ircode + MAX_PACKET_PAYLOAD * n, streamlen - 1);
                        SendCommand(pCh, m_sequence, CMD_PACKET, stream, streamlen);
                        free(stream);
                    }
                }
                result = true;
            }
        }
        Close();
        m_sequence ++;
    }
    m_lockDevice.Unlock();
    return result;
}

bool MEIZURemoter::ReceiveIRCode(BLEAddress addr){
    m_lockDevice.Lock();
    bool result = false;
    if(Connect(addr)){
        GATTRemoteService * pService = GetService(serviceUUID);
        if(pService != nullptr){
            GATTRemoteCharacteristic * pCh = pService->GetCharacteristic(characteristicUUID);
            pCh->RegNotify();
            SendCommand(pCh, ++m_sequence, CMD_RECV_IR);
            delay(20000);
            SendCommand(pCh, m_sequence, 0x0b);
            SendCommand(pCh, m_sequence, 0x06);
            pCh->UnregNotify();
        }
        Close();
    }
    m_lockDevice.Unlock();
    return result;
}

void MEIZURemoter::OnNotify(GATTRemoteCharacteristic * pCh, uint8_t * data, uint16_t length){
    ESP_LOGI(LOG_TAG, "OnNotify received %d bytes", length);
    ESP_LOG_BUFFER_HEX(LOG_TAG, data, length);
}

bool MEIZURemoter::SendCommand(GATTRemoteCharacteristic * pCh, uint8_t * data, uint16_t length){
    bool result = false;
    if(pCh != nullptr){
        result = Write(pCh, data, length, true);
    }
    return result;
}

bool MEIZURemoter::SendCommand(GATTRemoteCharacteristic * pCh, uint8_t sequence, uint8_t cmd, uint8_t * stream, uint16_t length){
    uint8_t * data = (uint8_t *)malloc(length + 4);
    data[0] = 0x55;
    data[1] = length + 3;
    data[2] = sequence;
    data[3] = cmd;
    if( data != nullptr && length > 0){
        memcpy(data + 4, stream, length);
    }
    bool result = SendCommand(pCh, data, length + 4);
    free(data); 
    return result;
}