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
#include "infrared.h"

/*
 * DEFINES
 ****************************************************************************************
 */


/*
 * FUNCTIONS
 ****************************************************************************************
 */
static void sysInit(void)
{
    // Todo config, if need
    // clk_en rst_req
}

static void devInit(void)
{
    uint16_t rsn = rstrsn();
    
    iwdt_disable();
    
    dbgInit();
    debug("Start(rsn:0x%X)...\r\n", rsn);
}

static void irTest(void)
{
    irInit();
    
    while(1)
    {
        irCmdSend(0x66);
        bootDelayMs(250);
        irCmdRepeat();
        bootDelayMs(150);
    }
}

int main(void)
{
    sysInit();
    devInit();
    
    irTest();
}
