/*
 * Created by @georgezhao2010
 * https://github.com/georgezhao2010/esp32_meizu_remoter_gateway/
 * involute@sina.com
 */
#include <nvs.h>
#include <nvs_flash.h>
#include <esp_log.h>
#include "nvsstorage.h"

#define LOG_TAG "nvsstorage"

bool nvs_read_addrcount(uint8_t * addrcount){
    nvs_handle_t nvs_handle_read;
    bool result = false;
    if(nvs_open("storage", NVS_READWRITE, &nvs_handle_read)== ESP_OK){
        if(nvs_get_u8(nvs_handle_read, "addrcount", addrcount) == ESP_OK){
            result = true;
        }
        nvs_close(nvs_handle_read);
    }
    return result;
}

bool nvs_clear_addrcount(){
    uint8_t count = 0;
    bool result = false;
    nvs_handle_t nvs_handle_write;
    if(nvs_open("storage", NVS_READWRITE, &nvs_handle_write) == ESP_OK){
        if(nvs_set_u8(nvs_handle_write, "addrcount", count) == ESP_OK){
            result = true;
        }
        nvs_close(nvs_handle_write);
    }
    return result;
}

bool nvs_read_addrs(void * addrs, size_t * len){
    bool result = false;
    nvs_handle_t nvs_handle_read;
    if(nvs_open("storage", NVS_READONLY, &nvs_handle_read) == ESP_OK){
        if(nvs_get_blob(nvs_handle_read, "addrs", addrs, len) == ESP_OK){
            result = true;
        }
        nvs_close(nvs_handle_read);
    }
    return result;
}

bool nvs_write_addrs(uint8_t count, const void * addrs, size_t len){
    bool result = false;
    nvs_handle_t nvs_handle_write;
    if(nvs_open("storage", NVS_READWRITE, &nvs_handle_write) == ESP_OK){
        if(nvs_set_u8(nvs_handle_write, "addrcount", count) == ESP_OK &&
                nvs_set_blob(nvs_handle_write, "addrs", addrs, len) == ESP_OK &&
                nvs_commit(nvs_handle_write) == ESP_OK){
            result = true;
        }
        nvs_close(nvs_handle_write);
    }
    return result;
}

bool nvs_read_interval(uint8_t * interval){
    bool result = false;
    nvs_handle_t nvs_handle_read;
    if(nvs_open("storage", NVS_READWRITE, &nvs_handle_read) == ESP_OK){
        if(nvs_get_u8(nvs_handle_read, "interval", interval) == ESP_OK){
            result = true;
        }
        nvs_close(nvs_handle_read);
    }
    return result;
}

bool nvs_write_interval(uint8_t interval){

    bool result = false;
    nvs_handle_t nvs_handle_write;
    if(nvs_open("storage", NVS_READWRITE, &nvs_handle_write) == ESP_OK){
        if(nvs_set_u8(nvs_handle_write, "interval", interval) == ESP_OK){
            result = true;
        }
        nvs_close(nvs_handle_write);
    }
    return result;
}

bool nvs_read_bindthrld(int8_t * threshold){
    bool result = false;
    nvs_handle_t nvs_handle_read;
    if(nvs_open("storage", NVS_READWRITE, &nvs_handle_read) == ESP_OK){
        if(nvs_get_i8(nvs_handle_read, "bindthrld", threshold) == ESP_OK){
           result = true;
        }
        nvs_close(nvs_handle_read);
    }
    return result;
}

bool nvs_write_bindthrld(int8_t threshold){
    bool result = false;
    nvs_handle_t nvs_handle_write;
    if(nvs_open("storage", NVS_READWRITE, &nvs_handle_write) == ESP_OK){
        if(nvs_set_i8(nvs_handle_write, "bindthrld", threshold) == ESP_OK){
           result = true;
        }
        nvs_close(nvs_handle_write);
    }
    return result;
}