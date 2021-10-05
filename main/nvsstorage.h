/*
 * Created by @georgezhao2010
 * https://github.com/georgezhao2010/esp32_meizu_remoter_gateway/
 * involute@sina.com
 */
#ifndef _NVS_STORAGE_H_
#define _NVS_STORAGE_H_

#ifdef __cplusplus
extern "C"{
#endif
bool nvs_read_addrcount(uint8_t * addrcount);
bool nvs_clear_addrcount();
bool nvs_read_addrs(void * addrs, size_t * len);
bool nvs_write_addrs(uint8_t count, const void * addrs, size_t len);
bool nvs_read_interval(uint8_t * interval);
bool nvs_write_interval(uint8_t interval);
bool nvs_read_bindthrld(int8_t * threshold);
bool nvs_write_bindthrld(int8_t threshold);
#ifdef __cplusplus
}
#endif

#endif