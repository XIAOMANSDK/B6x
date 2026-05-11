/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Main Entry of the application.
 *
 ****************************************************************************************
 */

#include "drvs.h"
#include "link_master.h"
#include "app_user.h"
#include "dbg.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/*
 * FUNCTIONS
 ****************************************************************************************
 */
void master_test_conf(void);

static void sysInit(void)
{
    SYS_CLK_ALTER();
    iwdt_disable();
    
    puya_enter_dual_read();
    rcc_fshclk_set(FSH_CLK_DPSC42);
}

static void devInit(void)
{
    uint16_t rsn = rstrsn();

    dbgInit();
    debug("Start(rsn:0x%X)...\r\n", rsn);

    link_master_init();
    master_test_conf();

    #if (CFG_USB)
    bootDelayMs(4);
    usbdInit();
    #endif
}

int main(void)
{
    sysInit();
    devInit();

    GLOBAL_INT_START();

    link_master_schedule();
}
