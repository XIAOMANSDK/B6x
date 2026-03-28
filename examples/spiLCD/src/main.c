/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Main Entry of the application.
 *
 ****************************************************************************************
 */

#include "b6x.h"
#include "drvs.h"
#include "dbg.h"
#include "lcd.h"


/*
 * DEFINES
 ****************************************************************************************
 */

#define SYS_TICK_DIV    (SYS_GET_CLOCK() / 100) // 10ms


/*
 * FUNCTIONS
 ****************************************************************************************
 */

volatile uint32_t _sysTickCnt;

void SysTick_Handler(void)
{
    _sysTickCnt++;
}

uint32_t micros(void)
{
    return _sysTickCnt * 10000;
}

void delayMs(uint32_t ms)
{
    uint32_t cur_tick = _sysTickCnt;
    uint32_t cnt_delay = ((ms + 9) / 10);

    while ((uint32_t)(_sysTickCnt - cur_tick) < cnt_delay);
}

static void sysInit(void)
{
    SYS_CLK_ALTER();
    puya_enter_dual_read();
    rcc_fshclk_set(FSH_CLK_DPSC64);

    iwdt_disable();

    /* set Priority for Systick Interrupt */
    NVIC_SetPriority(SysTick_IRQn, 1);
    _sysTickCnt = 0;
    SysTick_Config(SYS_TICK_DIV);

    GLOBAL_INT_START();
}

static void devInit(void)
{
    uint16_t rsn = rstrsn();

    dbgInit();
    debug("Start(rsn:0x%X, clk:%" PRIu32 ")...\r\n", rsn, rcc_sysclk_freq());
}

#if (DISP_LOGO)
#include "Logo.h"

void lcd_logo(void)
{
    lcd_draw_image((LCD_WIDTH-Logo_WIDTH)>>1/*x*/, (LCD_HEIGHT-Logo_HEIGHT)>>1/*y*/,
                    Logo_WIDTH, Logo_HEIGHT, Logo_DATA);
    lcd_wait_done();

    delayMs(1000);

    lcd_fill_color((LCD_WIDTH-Logo_WIDTH)>>1/*x*/, (LCD_HEIGHT-Logo_HEIGHT)>>1/*y*/,
                    Logo_WIDTH, Logo_HEIGHT, RGB565_BLACK);
    lcd_wait_done();
}
#else
#define lcd_logo()
#endif

int main(void)
{
    sysInit();
    devInit();

    lcd_init();

    lcd_logo();

    while (1)
    {
        #if (DISP_EYES)
        extern void eyes_loop(void);
        eyes_loop();
        #endif

        #if (DISP_SLIDE)
        extern void slide_loop(void);
        slide_loop();
        #endif

        #if (DISP_ROBOT)
        extern void robot_loop(void);
        robot_loop();
        #endif

    }
}
