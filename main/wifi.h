/*
 * Created by @georgezhao2010
 * https://github.com/georgezhao2010/esp32_meizu_remoter_gateway/
 * involute@sina.com
 */

#ifndef _WIFI_H_
#define _WIFI_H_

#ifdef __cplusplus
extern "C"{
#endif
    typedef void (*on_prov_gotip)();
    typedef void (*on_connect)(bool);
    bool wifi_init();
    bool wifi_provisioned(bool restore, bool *provisioned);
    bool start_softap_provisioning(on_prov_gotip cb);
    bool wifi_init_sta(on_connect cb);
    bool wifi_connected();
#ifdef __cplusplus
}
#endif

#endif