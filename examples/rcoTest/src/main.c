/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Main entry for RCO test application
 *
 * @details
 * System init -> Device init -> RCO calibration test
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

/**
 ****************************************************************************************
 * @brief System initialization
 ****************************************************************************************
 */
static void sysInit(void)
{
    // TODO: Add system clock configuration if needed
}

/**
 ****************************************************************************************
 * @brief Device initialization
 *
 * @details
 * - Read reset reason
 * - Disable watchdog
 * - Initialize debug interface
 * - Print startup banner
 ****************************************************************************************
 */
static void devInit(void)
{
    uint16_t rsn = rstrsn();

    iwdt_disable();

    dbgInit();
    debug("Start(rsn:0x%X)...\r\n", rsn);
}

extern void rcoTest(void);

/**
 ****************************************************************************************
 * @brief Main entry
 *
 * @return int Program exit status
 ****************************************************************************************
 */
int main(void)
{
    sysInit();
    devInit();

    rcoTest();
}
