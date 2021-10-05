/*
 * Created by @georgezhao2010
 * https://github.com/georgezhao2010/esp32_meizu_remoter_gateway/
 * involute@sina.com
 */
#include <esp_log.h>
#include <lwip/sockets.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <atomic>
#include "SocketClient.h"

#define LOG_TAG "SocketClient"


SocketClient::SocketClient(int sock, const char * pAddr, int nPort,  SocketServer * pServer, on_client_close on_close_cb,
                           void * pKernel, on_client_message on_message_cb){
    m_pKernel = pKernel;
    m_onmessage_cb = on_message_cb;
    m_socket = sock;
    int len = strlen(pAddr);
    m_pAddr = (char *)malloc(len + 1);
    memcpy(m_pAddr, pAddr, len + 1);
    m_nPort = nPort;
    struct timeval timeout = {CONFIG_SOCKET_TIMEOUT, 0};
    setsockopt(m_socket,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(struct timeval));
    m_pServer = pServer;
    m_onclose_cb = on_close_cb;
}

SocketClient::~SocketClient(){
    free(m_pAddr);
    if(m_socket > 0){
        shutdown(m_socket, SHUT_RDWR);
        closesocket(m_socket);
    }
}

void SocketClient::task_Receive(void * param){
    SocketClient * pClient = (SocketClient *)param;
    char buff[512];
    int counter = 0;
    while(true){
        int len = recv(pClient->m_socket, buff, sizeof(buff) - 1, 0);
        if(len > 0 ){
            counter = 0;
            buff[len] = 0;
            //ESP_LOGI(LOG_TAG, "Received message from %s:%d - %s", pClient->m_pAddr, pClient->m_nPort, buff);
            if(pClient->m_onmessage_cb != nullptr){
                pClient->m_onmessage_cb(pClient->m_pKernel, pClient, buff, len);
            }
        }
        else if(len == 0){ /*closed by client*/
            break;
        } else{ 
            if(errno == EAGAIN){
                if(++counter >= 4){/*timed out in 120 seconds*/
                    break;
                }
            }
            else{
                break;
            }
        }
    }
    pClient->m_onclose_cb(pClient->m_pServer, pClient);
    vTaskDelete(NULL);
}

void SocketClient::Send(char * pMessage, int nLen){
    send(m_socket, (void *)pMessage, nLen, 0);
    //ESP_LOGI(LOG_TAG, "Send message to %s:%d - %s", m_pAddr, m_nPort, pMessage);
}

void SocketClient::Start(){
    xTaskCreate(task_Receive, "task_Receive", 4096, this, tskIDLE_PRIORITY, NULL);
}