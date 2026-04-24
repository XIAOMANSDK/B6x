/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Empty project template
 *
 * @details
 * Minimal application skeleton for new B6x projects.
 * Copy this folder and modify as needed.
 *
 * Test flow:
 * 1. System clock configuration (sysInit)
 * 2. Device initialization: watchdog disable, debug UART (devInit)
 * 3. Main loop: user procedure placeholder
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
 * @brief System clock configuration (override as needed)
 ****************************************************************************************
 */
static void sysInit(void)
{
    // Add custom clock config here if needed
}

/**
 ****************************************************************************************
 * @brief Device and peripheral initialization
 ****************************************************************************************
 */
static void devInit(void)
{
    uint16_t rsn = rstrsn();
    UNUSED_PARAM(rsn);
    iwdt_disable();

    dbgInit();
    debug("_blank(rsn:0x%X)...\r\n", rsn);
}

/**
 ****************************************************************************************
 * @brief User procedure placeholder (called in main loop)
 ****************************************************************************************
 */
static void userProc(void)
{
    // Add user logic here
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
        userProc();
    }
}
