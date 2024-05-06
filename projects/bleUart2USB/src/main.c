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
#include "dbg.h"


/*
 * DEFINES
 ****************************************************************************************
 */


/*
 * FUNCTIONS
 ****************************************************************************************
 */

extern void usbdInit(void);
extern void user_procedure(void);

static void sysInit(void)
{
    iwdt_disable();

    // switch syclk to 48M for USB
    rcc_sysclk_set(SYS_CLK_48M);

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

    // enable USB clk and iopad
    rcc_usb_en();
    usbdInit();
    NVIC_EnableIRQ(USB_IRQn);

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
