/*
 * Created by @georgezhao2010
 * https://github.com/georgezhao2010/esp32_meizu_remoter_gateway/
 * involute@sina.com
 */
#ifndef _MDNSDM_H_
#define _MDNSDM_H_

#ifdef __cplusplus
extern "C"{
#endif

bool mdnsmd_init(void);
const char * mdns_get_serial();
const char * mdns_get_version();

#ifdef __cplusplus
}
#endif

#endif