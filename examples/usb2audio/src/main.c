/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief USB audio device test entry point
 *
 * @details
 * Test flow:
 * 1. Initialize system clock, debug UART, USB device, and microphone
 * 2. Enable global interrupts
 * 3. In the main loop, capture mic data and send via USB isochronous endpoint
 *
 * Select the demo mode via cfg.h: DEMO_AUDIO_MIC or DEMO_AUDIO_HID
 *
 ****************************************************************************************
 */

#include "b6x.h"
#include "drvs.h"
#include "dbg.h"
#include "micphone.h"
#include "usb_audio_test.h"

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
    debug("USB2AUDIO(rsn:0x%X)...\r\n", rsn);

    usbdInit();
    micInit();
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
