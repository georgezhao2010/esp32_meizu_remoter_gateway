/*
 * Created by @georgezhao2010
 * https://github.com/georgezhao2010/esp32_meizu_remoter_gateway/
 * involute@sina.com
 */
#ifndef _KERNEL_H_
#define _KERNEL_H_
#include <map>
#include <cjson.h>
#include "SocketServer.h"
#include "SocketClient.h"
#include "Remoter.h"
#include "RemoterInfo.h"
#include "SemaphoreLock.h"


enum class KernelMode
{
    Normal = 0,
    ReadyBind = 1,
    Bind = 2
};

class Kernel{
public:
    Kernel();
    ~Kernel();
    bool Init();
    void Loop();
    static void Restart();
private:
    SocketServer m_server;
    KernelMode m_mode;
    uint8_t m_nUpdateInterval;
    uint8_t m_timertik;
    static void OnClientMessage(void * pCaller, SocketClient * pClient, char * pMessage, int nLen);
    static void task_Restart(void * param);
    static void OnBindModeChanged(void * pCaller, bool bBind);
    static void OnDeviceFound(void * pCaller, esp_bd_addr_t * addr);
    static void OnRSSIRead(void * pCaller, esp_bd_addr_t * addr, int8_t rssi);
    static void OnTimer(void * param);
    static void * UpdateAllDevices(void * param);
    static void * JustUpdateDevices(void * param);
    static uint8_t * ReadStreamFromASC(char * asc);
    void SaveAddress();
    void BoardcastInfo(MEIZURemoterInfo * pInfo);
    char * process_message_configinfo();
    char * process_message_devices();
    char * process_message_sendir(cJSON * data);
    char * process_message_recvir(cJSON * data);
    char * process_message_bind_threshold(cJSON * data);
    char * process_message_bind();
    char * process_message_removebind(cJSON * data);
    char * process_message_subscribe(SocketClient * pClient);
    char * process_message_setinterval(cJSON * data);
    MEIZURemoter * m_pRemoterInterface;
    std::map<BLEAddress, MEIZURemoterInfo*> m_mapInfos;
    SemaphoreLock m_lock;
    esp_timer_handle_t m_timer;
    
};

#endif