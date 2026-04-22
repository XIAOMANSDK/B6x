/**
 ****************************************************************************************
 *
 * @file ws2812.h
 *
 * @brief WS2812 RGB LED driver interface.
 *
 ****************************************************************************************
 */

#ifndef WS2812_H_
#define WS2812_H_

#include <stdint.h>

typedef enum
{
    LED_MODE_IDLE = 0,
    LED_MODE_FAST,
    LED_MODE_SLOW,
    LED_MODE_CHARGING,
    LED_MODE_HOLD,

    LED_MODE_MAX,
} led_mode_t;

/**
 ****************************************************************************************
 * @brief Initialize WS2812 PWM+DMA driver and start animation scanner.
 ****************************************************************************************
 */
void ws2812_init(void);

/**
 ****************************************************************************************
 * @brief Start playing an LED animation mode.
 *
 * @param[in] mode  Animation mode index (see led_mode_t)
 ****************************************************************************************
 */
void ws2812_play(uint8_t mode);

#endif // WS2812_H_
