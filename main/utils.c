/*
 * Created by @georgezhao2010
 * https://github.com/georgezhao2010/esp32_meizu_remoter_gateway/
 * involute@sina.com
 */

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "utils.h"

void delay(int microseconds){
    vTaskDelay(microseconds / portTICK_PERIOD_MS);
}