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
#include "bledef.h"
#include "app.h"
#include "sftmr.h"
#include "leds.h"
#include "uartRb.h"
#include "atcmd.h"
#include "dbg.h"


/*
 * DEFINES
 ****************************************************************************************
 */


/*
 * FUNCTIONS
 ****************************************************************************************
 */

extern void user_procedure(void);

static void sysInit(void)
{
    rcc_sysclk_set(SYS_CLK);
    
    iwdt_disable();

    rcc_ble_en();
    rcc_adc_en();

    rcc_fshclk_set(FSH_CLK_DPSC42);
}

static void devInit(void)
{
    uint16_t rsn = rstrsn();
    
    uart1Rb_Init();
    debug("Start(rsn:%X, clk:%d)...\r\n", rsn, rcc_sysclk_freq());
    
    atConfigFlashRead();
    
    #if ((LED_PLAY) || (CFG_SFT_TMR))
    sftmr_init();
    
    #if (LED_PLAY)
    leds_init();
    leds_play(LED_FAST_BL);
    #endif //(LED_PLAY)
    
    #endif

    // Init BLE App
    app_init(rsn);
}

int main(void)
{
    sysInit();
    devInit();
    
    // Global Interrupt Enable
    GLOBAL_INT_START();
    
    // main loop
    while (1)
    {
        // Schedule Messages & Events
        ble_schedule();

        #if ((LED_PLAY) || (CFG_SFT_TMR))
        // SoftTimer Polling
        sftmr_schedule();
        #endif //(LED_PLAY)

        // User's Procedure
        user_procedure();
    }
}
