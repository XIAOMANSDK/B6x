/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief USB HID device test (boot keyboard/mouse or custom raw HID)
 *
 * @details
 * Test flow:
 * 1. Initialize system clock, debug UART, and USB HID device
 * 2. Enable global interrupts
 * 3. In the main loop, send HID reports (keyboard/mouse or custom raw data)
 *
 * Select the demo mode via cfg.h: DEMO_HID_BOOT or DEMO_HID_CUSTOM
 *
 ****************************************************************************************
 */

#include "b6x.h"
#include "drvs.h"
#include "dbg.h"
#include "usb_hid_test.h"

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
    debug("USB2HID(rsn:0x%X)...\r\n", rsn);

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
