/*
 * Created by @georgezhao2010
 * https://github.com/georgezhao2010/esp32_meizu_remoter_gateway/
 * involute@sina.com
 */

#ifndef _SOCKET_CLIENT_H_
#define _SOCKET_CLIENT_H_

class SocketServer;
class SocketClient;
typedef void (* on_client_close)(SocketServer * pServer, SocketClient * pClient);
typedef void (* on_client_message)(void * pCaller, SocketClient * pClient, char * pMessage, int nLen);

class SocketClient{
public:
    SocketClient(int sock, const char * pAddr, int nPort,  SocketServer * pServer, on_client_close on_close_cb, 
                 void * pKernel, on_client_message on_message_cb);
    ~SocketClient();
    inline const char * GetPeerAddr(){return m_pAddr;}
    inline int GetPeerPort(){return m_nPort;}
    void Send(char * pMessage, int nLen);
    void Start();
private:
    void * m_pKernel;
    int m_socket;
    char * m_pAddr;
    int m_nPort;
    SocketServer * m_pServer;
    on_client_close m_onclose_cb;
    on_client_message m_onmessage_cb;
    static void task_Receive(void * param);
    
};

#endif