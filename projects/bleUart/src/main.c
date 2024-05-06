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
    APBMISC->XOSC16M_CTRL.XOSC16M_CAP_TR = 0x22;
}

static void devInit(void)
{
    uint16_t rsn = rstrsn();

    uart1Rb_Init(); //dbgInit();
    debug("Start(rsn:%X)...\r\n", rsn);

    // Init BLE App
    app_init(rsn);

    rf_pa_set(0x0C);
    #if (LED_PLAY)
    sftmr_init();
    leds_init();
    leds_play(LED_FAST_BL);
    #endif //(LED_PLAY)
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
