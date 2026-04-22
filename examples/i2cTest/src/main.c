/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Main Entry of the application - I2C test
 *
 * @details
 * Program flow:
 * 1. System init: configure clock
 * 2. Device init: disable watchdog, init debug
 * 3. Run I2C test (master or slave mode per cfg.h)
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
 * @brief System initialization
 ****************************************************************************************
 */
static void sys_init(void)
{
    // Todo config, if need
    rcc_sysclk_set(SYS_CLK);
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

extern void i2c_test(void);

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

    i2c_test();
}
