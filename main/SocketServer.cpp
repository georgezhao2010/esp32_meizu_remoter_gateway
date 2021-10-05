/*
 * Created by @georgezhao2010
 * https://github.com/georgezhao2010/esp32_meizu_remoter_gateway/
 * involute@sina.com
 */

#include <esp_log.h>
#include <lwip/sockets.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "SocketServer.h"
#include "SocketClient.h"

#define LOG_TAG "SocketServer"

SocketServer::SocketServer(){
    m_pKernel = nullptr;
    m_client_message_cb = nullptr;
    m_socket = -1;
}

SocketServer::~SocketServer(){

}

bool SocketServer::Listen(int nPort){
    int addr_family;
    int ip_protocol;
#ifdef CONFIG_SOCKET_USE_IPV6
    struct sockaddr_in6 addr;
    bzero(&addr.sin6_addr.un, sizeof(addr.sin6_addr.un));
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(nPort);
    addr_family = AF_INET6;
    ip_protocol = IPPROTO_IPV6;
#else
    struct sockaddr_in addr;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(nPort);
    addr_family = AF_INET;
    ip_protocol = IPPROTO_IP;
#endif
    m_socket = socket(addr_family, SOCK_STREAM, ip_protocol);
    if(m_socket < 0){
        ESP_LOGE(LOG_TAG, "Uncble to create socket, errorno:%d", errno);
        return false;
    }
    int ret = bind(m_socket, (struct  sockaddr *)&addr, sizeof(addr));
    if(ret != ESP_OK){
        ESP_LOGE(LOG_TAG, "Socket bind error, errno: %d", errno);
        return false;
    }
    ret = listen(m_socket, 5);
    if(ret != ESP_OK){
        ESP_LOGE(LOG_TAG, "Faild to during listen, errno: %d", errno);
        return false;
    }
    ESP_LOGD(LOG_TAG, "Listening on port %d", CONFIG_SOCKET_LISTEN_PORT);
    xTaskCreate(task_Listen, "task_Listen", 4096, this, tskIDLE_PRIORITY, NULL);

    return true;
}

bool SocketServer::Close(){
    return false;
}

SocketClient * SocketServer::AddClient(int sock, const char * addr_str, int port, SocketServer *pServer){
    SocketClient * pClient = new SocketClient(sock, addr_str, port, pServer, 
        OnClientClose, m_pKernel, m_client_message_cb);
    m_clients.insert(std::pair<SocketClient *, bool>(pClient, false));
    return pClient;
}

void SocketServer::RemoveClient(SocketClient * pClient){
    auto it = m_clients.find(pClient);
    if(it != m_clients.end()){
        m_clients.erase(it);
        delete pClient;
    }
}

void SocketServer::task_Listen(void * param){
    SocketServer * pServer = (SocketServer *)param;
    char addr_str[128];
    while(pServer->m_socket > 0){
#ifdef CONFIG_SOCKET_USE_IPV6
        struct sockaddr_in6 cliaddr;
#else
        struct sockaddr_in cliaddr;
#endif
        uint addrLen = sizeof(cliaddr);
        int sock = accept(pServer->m_socket, (struct  sockaddr *)&cliaddr, &addrLen);
        if(sock < 0){
            ESP_LOGE(LOG_TAG, "Faild to accept connection, errno: %d", errno);
            continue;
        }
#ifdef CONFIG_SOCKET_USE_IPV6
        inet6_ntoa_r(cliaddr.sin6_addr, addr_str, sizeof(addr_str) - 1);
#else
        inet_ntoa_r(cliaddr.sin_addr, addr_str, sizeof(addr_str) - 1);
#endif
        int port = (int)ntohs(cliaddr.sin_port);
        pServer->m_lock.Lock();
        SocketClient * pClient = pServer->AddClient(sock, addr_str, port, pServer);
        ESP_LOGI(LOG_TAG, "The client at %s:%d was connected, total clients %d", 
            pClient->GetPeerAddr(), pClient->GetPeerPort(), pServer->m_clients.size());
        pClient->Start();
        pServer->m_lock.Unlock();
        
    }
}

void SocketServer::OnClientClose(SocketServer * pServer, SocketClient * pClient){
    pServer->m_lock.Lock();
    ESP_LOGI(LOG_TAG, "The client at %s:%d was disconnected", 
        pClient->GetPeerAddr(), pClient->GetPeerPort());
    pServer->RemoveClient(pClient);
    
    pServer->m_lock.Unlock();
}

void SocketServer::SendMessage(SocketClient * pClient, char * pMessage, int nLen){
    m_lock.Lock();
    auto it = m_clients.find(pClient);
    if(it != m_clients.end()){
        pClient->Send(pMessage, nLen);
    }
    m_lock.Unlock();
}

int SocketServer::Boardcast(char * pMessage, int nLen){
    m_lock.Lock();
    int counter = 0;
    for(auto it: m_clients){
        if(it.second){
            it.first->Send(pMessage, nLen);
            counter ++;
        }
    }
    m_lock.Unlock();
    return counter;
}

void SocketServer::Subscribe(SocketClient * pClient){
    m_lock.Lock();
    auto it = m_clients.find(pClient);
    if(it != m_clients.end()){
        it->second = true;
    }
    m_lock.Unlock();
}