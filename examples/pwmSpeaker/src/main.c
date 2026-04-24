/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Main Entry of the application - PWM Speaker test
 *
 ****************************************************************************************
 */

#include "b6x.h"
#include "drvs.h"
#include "dbg.h"


/*
 * FUNCTIONS
 ****************************************************************************************
 */

extern void speaker_pwm_init(void);
extern void speaker_play(void);

/**
 ****************************************************************************************
 * @brief System initialization
 ****************************************************************************************
 */
static void sys_init(void)
{
    SYS_CLK_ALTER();
    rcc_fshclk_set(FSH_CLK_DPSC64);
    boya_flash_quad_mode();
    boya_enter_hpm();
}

/**
 ****************************************************************************************
 * @brief Device initialization
 *
 * @details Disable watchdog, initialize debug interface
 ****************************************************************************************
 */
static void dev_init(void)
{
    iwdt_disable();

    dbgInit();
}

/**
 ****************************************************************************************
 * @brief Main entry
 * @return Program exit code (never returns)
 ****************************************************************************************
 */
int main(void)
{
    sys_init();
    dev_init();

    speaker_pwm_init();

    while (1)
    {
        speaker_play();

        bootDelayMs(2000);
    }
}
