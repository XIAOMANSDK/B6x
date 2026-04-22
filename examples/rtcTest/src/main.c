/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Main entry for RTC test application.
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

static void sys_init(void)
{
    SYS_CLK_ALTER();
}

static void dev_init(void)
{
    uint16_t rsn = rstrsn();

    iwdt_disable();

    dbgInit();
    debug("Start(rsn:0x%X)...\r\n", rsn);
}

extern void rtcTest(void);

int main(void)
{
    sys_init();
    dev_init();

    rtcTest();
}
