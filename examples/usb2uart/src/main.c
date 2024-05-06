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
extern void usbdTest(void);

static void sysInit(void)
{
    // switch syclk to 48M for USB
    rcc_sysclk_set(SYS_CLK_48M);

    // enable USB clk and iopad
    rcc_usb_en();
}

static void devInit(void)
{
    uint16_t rsn = rstrsn();
    
    iwdt_disable();
    
    dbgInit();
    debug("Start(rsn:0x%X)...\r\n", rsn);
    
    usbdInit();
}

int main(void)
{
    sysInit();
    devInit();

    NVIC_EnableIRQ(USB_IRQn);
    __enable_irq();

    while (1)
    {
        usbdTest();
    }
}
