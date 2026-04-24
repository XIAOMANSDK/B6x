/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Main Entry of the BLE 3-Master + 3-Slave multi-connection application.
 *
 ****************************************************************************************
 */

#include "b6x.h"
#include "bledef.h"
#include "drvs.h"
#include "app.h"
#include "keys.h"
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

/**
 ****************************************************************************************
 *
 * @brief Initialize system clocks and enable peripheral modules.
 *
 * Enables BLE, ADC clocks and configures flash wait states.
 *
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
 *
 * @brief Initialize device peripherals and BLE application.
 *
 * @param[in] rsn  Reset reason from hardware register.
 *
 ****************************************************************************************
 */
static void devInit(void)
{
    uint16_t rsn = rstrsn();

    uart1Rb_Init();
    debug("Start(rsn:%X)...\r\n", rsn);

    keys_init();

    // Init BLE App
    app_init(rsn);
}

/**
 ****************************************************************************************
 *
 * @brief Application entry point.
 *
 * Initializes hardware and BLE stack, then enters main event loop.
 *
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

        // User's Procedure
        user_procedure();
    }
}
