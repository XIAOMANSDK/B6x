/**
 ****************************************************************************************
 *
 * @file slide.c
 *
 * @brief Animated slide
 *
 ****************************************************************************************
 */

#if (DISP_SLIDE)

#include "lcd.h"
#include "slide\coffee.h"
#include "slide\coffee_font.h"
#include "slide\steam.h"
#include "slide\steam_font.h"


#define IMAGE_WIDTH     240
#define IMAGE_HEIGHT    164

#define FONT_WIDTH      66
#define FONT_HEIGHT     18


extern void delay_ms(uint32_t ms);

void slide_loop(void)
{
    lcd_draw_image(0, 76, IMAGE_WIDTH, IMAGE_HEIGHT, gImage_steam);
    lcd_wait_done();
    lcd_draw_image(87, 30, FONT_WIDTH, FONT_HEIGHT, gImage_steam_font);
    lcd_wait_done();
    delay_ms(1000);

    lcd_draw_image(0, 76, IMAGE_WIDTH, IMAGE_HEIGHT, gImage_coffee);
    lcd_wait_done();
    lcd_draw_image(87, 30, FONT_WIDTH, FONT_HEIGHT, gImage_coffee_font);
    lcd_wait_done();
    delay_ms(1000);
}

#endif
