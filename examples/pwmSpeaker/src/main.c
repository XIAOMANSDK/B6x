/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief PWM≤•∑≈”Ô“Ù æ¿˝.
 *
 ****************************************************************************************
 */

#include "b6x.h"
#include "drvs.h"
#include "dbg.h"

/*
 * DEFINES
 ****************************************************************************************
 */
extern void pwmInit(void);
extern void speakerPlay(void);

static void sysInit(void)
{
    // Todo config, if need
    SYS_CLK_ALTER();
    rcc_fshclk_set(FSH_CLK_DPSC64);
    boya_flash_quad_mode();
    boya_enter_hpm();
}

static void devInit(void)
{
    iwdt_disable();
    
    dbgInit();
}

int main(void)
{
    sysInit();
    devInit();
    
    pwmInit();
    
    while (1)    
    {
        speakerPlay();
        
        bootDelayMs(2000);
    }
}
