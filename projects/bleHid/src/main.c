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
#include "bledef.h"
#include "drvs.h"
#include "app.h"
#include "sftmr.h"
#include "leds.h"
#include "batt.h"
#include "uartRb.h"
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
    iwdt_disable();

    rcc_ble_en();
    rcc_adc_en();

    rcc_fshclk_set(FSH_CLK_DPSC42);
}

static void devInit(void)
{
    uint16_t rsn = rstrsn();

    uart1Rb_Init(); //dbgInit();
    debug("Start(rsn:%X)...\r\n", rsn);

    #if (LED_PLAY)
    sftmr_init();
    leds_init();
    leds_play(LED_FAST_BL);
    #endif //(LED_PLAY)

    batt_init();

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

        #if (LED_PLAY)
        // SoftTimer Polling
        sftmr_schedule();
        #endif //(LED_PLAY)

        // User's Procedure
        user_procedure();
    }
}
