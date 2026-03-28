#ifndef WS2812_H_
#define WS2812_H_

#include <stdint.h>

enum led_mode
{
    LED_MODE_IDLE = 0,
    LED_MODE_FAST,
    LED_MODE_SLOW,
    LED_MODE_CHARGING,
    LED_MODE_HOLD,

    LED_MODE_MAX,
};

void ws2812_init(void);
void ws2812_play(uint8_t mode);
#endif // WS2812_H_
