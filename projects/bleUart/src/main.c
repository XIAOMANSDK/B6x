/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Main entry of the BLE UART transparent transmission application.
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

#include "proc.h"

/*
 * DEFINES
 ****************************************************************************************
 */


/*
 * FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialize system clocks and enable necessary peripherals.
 ****************************************************************************************
 */
static void sysInit(void)
{
    iwdt_disable();

    rcc_ble_en();
    rcc_adc_en();

    rcc_fshclk_set(FSH_CLK_DPSC42);
}

/**
 ****************************************************************************************
 * @brief Initialize device peripherals and BLE application.
 ****************************************************************************************
 */
static void devInit(void)
{
    uint16_t rsn = rstrsn();

    uart1Rb_Init(); //dbgInit();
    debug("Start(rsn:%X)...\r\n", rsn);

    // Init BLE App
    app_init(rsn);

    #if (LED_PLAY)
    sftmr_init();
    leds_init();
    leds_play(LED_FAST_BL);
    #endif //(LED_PLAY)
}

/**
 ****************************************************************************************
 * @brief Application entry point.
 ****************************************************************************************
 */
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
