/*
 * Created by @georgezhao2010
 * https://github.com/georgezhao2010/esp32_meizu_remoter_gateway/
 * involute@sina.com
 */
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include "led.h"
#include "utils.h"

#define LOG_TAG "led"

struct led_status{
    int mode;
    int duration;
    struct led_status * pprev;
};
struct led_status head = {
    .mode = LED_MODE_OFF,
    .duration = 0,
    .pprev = NULL
};
struct led_status * ptail = &head;
bool led_on = false;
bool thread_is_running = false;

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

bool remove_outdate(){
    bool bchange = false;
    taskENTER_CRITICAL(&mux);
    struct led_status * pcur = ptail;
    while(pcur != &head){
        if(pcur->duration > 0){
            pcur->duration--;
        }
        if(pcur->duration == 0){
            struct led_status * pold = pcur;
            if(ptail == pcur){
                ptail = pcur->pprev;
            }
            bchange = true;
            pcur = pcur->pprev;
            free(pold);
        } else {
            pcur = pcur->pprev;
        }
    }
    taskEXIT_CRITICAL(&mux);
    return bchange;
}

void turn_on(){
    if(!led_on){
        gpio_set_level(CONFIG_LED_GPIO, 1);
        led_on = true;
    }
}

void turn_off(){
    if(led_on){
        gpio_set_level(CONFIG_LED_GPIO, 0);
        led_on = false;
    }
}

static void task_thread(void * arg){
    int counter;
    bool brun = true;
    taskENTER_CRITICAL(&mux);
    thread_is_running = true;
    taskEXIT_CRITICAL(&mux);
    
    for(counter = 0; brun; counter++){
        taskENTER_CRITICAL(&mux);
        if(ptail == &head && (ptail->mode == LED_MODE_OFF || ptail->mode == LED_MODE_ON)){
            brun = false;
        }
        switch(ptail->mode){
            case LED_MODE_OFF:
                turn_off();
                break;
            case LED_MODE_ON:
                turn_on();
                break;
            case LED_MODE_SLOW_BLINK:
                if(counter == CONFIG_LED_SLOW_BLINK_CYCLE * 2){
                    counter = 0;
                }
                if(counter / CONFIG_LED_SLOW_BLINK_CYCLE == 0 && led_on){
                    turn_off();
                }
                else if(counter / CONFIG_LED_SLOW_BLINK_CYCLE == 1 && !led_on){
                    turn_on();
                }
                break;
            case LED_MODE_FAST_BLINK:
                if(counter == CONFIG_LED_FAST_BLINK_CYCLE * 2){
                    counter = 0;
                }
                if(counter / CONFIG_LED_FAST_BLINK_CYCLE >= 2){
                    counter = 0;
                }
                if(counter / CONFIG_LED_FAST_BLINK_CYCLE == 0 && led_on){
                    turn_off();
                }
                else if(counter / CONFIG_LED_FAST_BLINK_CYCLE == 1 && !led_on){
                    turn_on();
                }
                break;
        }
        taskEXIT_CRITICAL(&mux);
        if(brun){
            delay(100);
            if(remove_outdate()){
                counter = 0;
            }
        }
    }
    taskENTER_CRITICAL(&mux);
    thread_is_running = false;
    taskEXIT_CRITICAL(&mux);
    vTaskDelete(NULL);
}

bool led_set_mode(int mode, int duration){
    taskENTER_CRITICAL(&mux);
    if(duration == 0){
        head.mode = mode;
        if(!thread_is_running){
            switch(mode){
                case LED_MODE_ON:
                    turn_on();
                    break;
                case LED_MODE_OFF:  
                    turn_off();
                    break;
                case LED_MODE_SLOW_BLINK:
                case LED_MODE_FAST_BLINK:
                    xTaskCreate(task_thread, "task_thread", 2048, NULL, tskIDLE_PRIORITY, NULL);
                    break;
            }
        }
    } else {
        struct led_status * pnew = (struct led_status *)malloc(sizeof(struct led_status));
        pnew->mode = mode;
        pnew->duration = duration * 10;
        pnew->pprev = ptail;
        ptail = pnew;
        if(!thread_is_running){
            xTaskCreate(task_thread, "task_thread", 2048, NULL, tskIDLE_PRIORITY, NULL);
        }
    }
    taskEXIT_CRITICAL(&mux);
    return true;
}

bool led_init(){
    gpio_set_direction(CONFIG_LED_GPIO, GPIO_MODE_OUTPUT);
    led_set_mode(LED_MODE_OFF, 0);
    return true;
}
