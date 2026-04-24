/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Main Entry of the application.
 *
 ****************************************************************************************
 */

#include "regs.h"
#include "drvs.h"
#include "sysdbg.h"
#include "dbg.h"

#include "link_master.h"
#include "link_slave.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/*
 * FUNCTIONS
 ****************************************************************************************
 */
extern void master_test_conf(void);
extern void slave_test_conf(void);

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

    link_slave_init();
    slave_test_conf();
}

int main(void)
{   
    sysInit();
    devInit();
  
    GLOBAL_INT_START();

    link_slave_schedule();
}
