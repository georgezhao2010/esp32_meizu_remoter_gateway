/*
 * Created by @georgezhao2010
 * https://github.com/georgezhao2010/esp32_meizu_remoter_gateway/
 * involute@sina.com
 */
#ifndef _SEMAPHORE_LOCK_H_
#define _SEMAPHORE_LOCK_H_
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

class SemaphoreLock{
public:
    SemaphoreLock(bool initLock = false);
    ~SemaphoreLock();
    void Lock();
    void Unlock();
private:
    SemaphoreHandle_t m_semaphore;
};


#endif