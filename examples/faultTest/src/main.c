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

#if (DBG_MODE != DBG_VIA_UART)
#error "Trace info output by printf()."
#endif


/*
 * FUNCTIONS
 ****************************************************************************************
 */

static void sysInit(void)
{
    // Todo config, if need
    
}

static void devInit(void)
{
    uint16_t rsn = rstrsn();
    
    iwdt_disable();
    
    dbgInit();
    uint32_t fault_lr = __return_address();
    uint32_t fault_sp = __current_sp();
    debug("Start(rsn:0x%X)...\r\n", rsn);
    
    debug("%x, %x...\r\n", fault_lr, fault_sp);
}

static void userProc(void)
{
    // Todo user procedure
    
}

void faultTest(void)
{
    // init trace
    trace_init();
    
    // HardFault - unalign read
    volatile int * p;
    volatile int value;

    p = (int *) 0x00;
    value = *p;
    debug("addr:0x%02X value:0x%08X\r\n", (int) p, value);

    p = (int *) 0x04;
    value = *p;
    debug("addr:0x%02X value:0x%08X\r\n", (int) p, value);

    p = (int *) 0x03;
    value = *p;
    debug("addr:0x%02X value:0x%08X\r\n", (int) p, value);
}

int main(void)
{
    sysInit();
    devInit();
    
    faultTest();
    while (1)
    {
        userProc();
    }
}
