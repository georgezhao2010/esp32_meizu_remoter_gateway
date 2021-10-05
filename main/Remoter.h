/*
 * Created by @georgezhao2010
 * https://github.com/georgezhao2010/esp32_meizu_remoter_gateway/
 * involute@sina.com
 */
#ifndef _REMOTER_H_
#define _REMOTER_H_
#include "GATTDevice.h"
#include "RemoterInfo.h"

typedef void (*on_update_cb)(void *, MEIZURemoterInfo *);
typedef void (*on_ir_complete_cb)(void *, bool);

class MEIZURemoter: public GATTDevice{
public:
    MEIZURemoter();
    ~MEIZURemoter();
    bool ReadAllInfo(MEIZURemoterInfo * pInfo);
    bool SendIRCode(BLEAddress addr, uint8_t * key, uint16_t keylen, uint8_t * ircode, uint16_t ircodelen);
    bool ReceiveIRCode(BLEAddress addr);
    virtual void OnNotify(GATTRemoteCharacteristic * pCh, uint8_t * data, uint16_t length);
private:
    bool SendCommand(GATTRemoteCharacteristic * pCh, uint8_t * data, uint16_t length);
    bool SendCommand(GATTRemoteCharacteristic * pCh, uint8_t sequence, uint8_t cmd, uint8_t * stream = nullptr, uint16_t length = 0);
    uint8_t m_sequence;
    SemaphoreLock m_lockDevice;;
};

#endif