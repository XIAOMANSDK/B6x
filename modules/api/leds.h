/**
 ****************************************************************************************
 *
 * @file leds.h
 *
 * @brief Header file - Separate LED Display Module
 *
 ****************************************************************************************
 */

#ifndef _LEDS_H_
#define _LEDS_H_

#include <stdint.h>


/*
 * DEFINES
 ****************************************************************************************
 */

/// mode of leds_play *User should override it*
enum led_mode
{
    LED_SLOW_BL,  // slow blink
    LED_FAST_BL,  // fast blink
    LED_BUSY_BL,  // busy blink, more fast
    LED_CONT_ON,  // continued ON
    LED_HINT_BL,  // twice blink as hint
    
    // add more...
    
    LED_MODE_MAX
};


/*
 * FUNCTION DECLARATION
 ****************************************************************************************
 */

#if (LED_PLAY)
/// Init iopad and env
void leds_init(void);

/// Dispaly 'mode' @see enum led_mode
void leds_play(uint8_t mode);
#else
/// Disable via empty marco
#define leds_init()
#define leds_play(mode)
#endif


#endif // _LEDS_H_
