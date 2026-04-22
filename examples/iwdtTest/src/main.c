/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Independent watchdog (IWDT) test - interrupt-mode feeding
 *
 * @details
 * Test flow:
 * 1. Configure IWDT with interrupt mode, 1-second timeout (LSI 32.768 kHz)
 * 2. ISR feeds the watchdog on each timeout and sets event flag
 * 3. Main loop logs feed count via debug interface
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

/// Watchdog timeout window: 32768 counts at 32.768 kHz LSI = 1 second
#define IWDT_WINDOW        (32768)

/*
 * VARIABLES
 ****************************************************************************************
 */

static volatile bool irq_evt = false;   ///< IWDT interrupt event flag
static volatile uint16_t irq_cnt = 0;   ///< IWDT feed counter

/*
 * FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief IWDT interrupt handler
 *
 * @details
 * Feed watchdog and set event flag for main loop processing.
 ****************************************************************************************
 */
void IWDT_IRQHandler(void)
{
    iwdt_feed();
    irq_evt = true;
    irq_cnt++;
}

/**
 ****************************************************************************************
 * @brief Initialize independent watchdog
 *
 * @details
 * Enable IWDT with interrupt + reset, LSI clock source, and 1-second window.
 ****************************************************************************************
 */
static void iwdtInit(void)
{
    iwdt_init(IWDT_INTEN_BIT | IWDT_CR_DFLT);
    iwdt_conf(IWDT_WINDOW);

    NVIC_EnableIRQ(IWDT_IRQn);
    __enable_irq();
}

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
 * @brief Device initialization and IWDT setup
 ****************************************************************************************
 */
static void devInit(void)
{
    uint16_t rsn = rstrsn();

    dbgInit();
    debug("Start(rsn:0x%X)...\r\n", rsn);

    iwdtInit();
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

    while (1)
    {
        if (irq_evt)
        {
            irq_evt = false;
            debug("iwdt irq:%d\r\n", irq_cnt);
        }
    }
}
