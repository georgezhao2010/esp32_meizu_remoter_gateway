/*
 * Created by @georgezhao2010
 * https://github.com/georgezhao2010/esp32_meizu_remoter_gateway/
 * involute@sina.com
 */

#ifndef _SOCKET_SERVER_H_
#define _SOCKET_SERVER_H_

#include "SemaphoreLock.h"
#include <map>

class SocketClient;


typedef void (* on_client_message)(void * pCaller, SocketClient * pClient, char * pMessage, int nLen);

class SocketServer{
public:
    SocketServer();
    ~SocketServer();
    inline void SetCallback(void * pCaller, on_client_message cb){
        m_pKernel = pCaller;
        m_client_message_cb = cb;
    }
    bool Listen(int nPort);
    int Boardcast(char * pMessage, int nLen);
    void SendMessage(SocketClient * pClient, char * pMessage, int nLen);
    void Subscribe(SocketClient * pClient);
    bool Close();
private:
    int m_socket;
    static void OnClientClose(SocketServer * pServer, SocketClient * pClient);
    static void task_Listen(void * param);
    SocketClient * AddClient(int sock, const char * addr_str, int port, SocketServer *pServer);
    void RemoveClient(SocketClient * pClient);
    std::map<SocketClient *, bool> m_clients;
    SemaphoreLock m_lock;
    void * m_pKernel;
    on_client_message m_client_message_cb;
};

#endif