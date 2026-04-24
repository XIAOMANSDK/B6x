/**
 ****************************************************************************************
 *
 * @file robot.c
 *
 * @brief Animated robot eyes, refer to https://github.com/upiir/dual_lcd_robot_eyes
 *
 ****************************************************************************************
 */

#if (DISP_ROBOT)

#include "lcd.h"
#include "robot\robot_eyes.h"

#define EYES_WIDTH          160
#define EYES_HEIGHT         160
#define EYES_CNT            3

const uint16_t* robot_eyes_images[EYES_CNT] = {
    #if (0)
    image_data_robot_eyes_07,
    image_data_robot_eyes_12,
    image_data_robot_eyes_21,
    #else
    image_data_robot_eyes_14,
    image_data_robot_eyes_22,
    image_data_robot_eyes_06,
    #endif
};

extern void delay_ms(uint32_t ms);

void robot_loop(void)
{
    lcd_select(ID_LCDS);
    lcd_draw_image(40, 40, EYES_WIDTH, EYES_HEIGHT, robot_eyes_images[0]);
    lcd_wait_done();
    delay_ms(1000);
    
    lcd_select(ID_LCD1);
    lcd_draw_image(40, 40, EYES_WIDTH, EYES_HEIGHT, robot_eyes_images[1]);
    lcd_wait_done();
    delay_ms(500);

    lcd_select(ID_LCD2);
    lcd_draw_image(40, 40, EYES_WIDTH, EYES_HEIGHT, robot_eyes_images[2]);
    lcd_wait_done();
    delay_ms(1000);

    lcd_select(ID_LCDS);
    for (int i = 0; i < EYES_CNT; i++) {
        lcd_draw_image(40, 40, EYES_WIDTH, EYES_HEIGHT, (const uint8_t *)robot_eyes_images[i]);
        lcd_wait_done();
        delay_ms(1000);
    }
}

#endif
