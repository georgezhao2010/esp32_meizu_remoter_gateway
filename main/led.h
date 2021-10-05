/*
 * Created by @georgezhao2010
 * https://github.com/georgezhao2010/esp32_meizu_remoter_gateway/
 * involute@sina.com
 */

#ifndef _LED_H_
#define _LED_H_

#define LED_MODE_OFF 0
#define LED_MODE_ON 1
#define LED_MODE_SLOW_BLINK 2
#define LED_MODE_FAST_BLINK 3

#ifdef __cplusplus
extern "C"{
#endif
bool led_init();
bool led_set_mode(int mode, int duration);

#ifdef __cplusplus
}
#endif

#endif