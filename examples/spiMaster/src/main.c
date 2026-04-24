/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Main Entry of the application - SPI Master test
 *
 ****************************************************************************************
 */

#include "b6x.h"
#include "drvs.h"
#include "dbg.h"


/*
 * FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief System initialization (template placeholder)
 ****************************************************************************************
 */
static void sys_init(void)
{
    // Todo config, if need
    SYS_CLK_ALTER();

    rcc_adc_en();
}

/**
 ****************************************************************************************
 * @brief Device initialization
 *
 * @details Disable watchdog, initialize debug interface
 ****************************************************************************************
 */
static void dev_init(void)
{
    iwdt_disable();

    dbgInit();
}

extern void spim_test(void);

/**
 ****************************************************************************************
 * @brief Main entry
 * @return Program exit code (never returns)
 ****************************************************************************************
 */
int main(void)
{
    sys_init();
    dev_init();

    spim_test();
}
