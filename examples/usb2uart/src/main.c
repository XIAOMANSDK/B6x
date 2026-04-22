/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief USB CDC virtual serial port (USB-to-UART bridge)
 *
 * @details
 * Test flow:
 * 1. Initialize system clock, debug UART, and USB device
 * 2. Enable global interrupts
 * 3. In the main loop, send test data when the host opens the virtual COM port
 *
 ****************************************************************************************
 */

#include "b6x.h"
#include "drvs.h"
#include "dbg.h"
#include "cdc_uart.h"

/*
 * FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief System clock configuration
 ****************************************************************************************
 */
static void sysInit(void)
{
    SYS_CLK_ALTER();
}

/**
 ****************************************************************************************
 * @brief Device and peripheral initialization
 ****************************************************************************************
 */
static void devInit(void)
{
    uint16_t rsn = rstrsn();

    iwdt_disable();

    dbgInit();
    debug("USB2UART(rsn:0x%X)...\r\n", rsn);

    usbdInit();
}

/**
 ****************************************************************************************
 * @brief Application entry point
 ****************************************************************************************
 */
int main(void)
{
    sysInit();
    devInit();

    GLOBAL_INT_START();

    while (1)
    {
        usbdTest();
    }
}
