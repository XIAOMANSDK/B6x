/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Main Entry of the application - Infrared TX test
 *
 * @details
 * Continuously sends IR command and repeat codes using the ATMR module.
 * - irCmdSend: sends a complete NEC frame (~110ms)
 * - irCmdRepeat: sends a repeat code
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

#define IR_TEST_CMD             (0x66)
#define IR_TEST_SEND_DELAY_MS   (250)
#define IR_TEST_REPEAT_DELAY_MS (150)


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
    // clk_en rst_req
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
    uint16_t rsn = rstrsn();

    iwdt_disable();

    dbgInit();
    debug("Start(rsn:0x%X)...\r\n", rsn);
}

/**
 ****************************************************************************************
 * @brief Infrared TX test function
 *
 * @details
 * Initialize IR module, then loop sending command and repeat codes
 * with fixed delays between transmissions.
 ****************************************************************************************
 */
static void ir_test(void)
{
    irInit();

    while (1)
    {
        irCmdSend(IR_TEST_CMD);
        bootDelayMs(IR_TEST_SEND_DELAY_MS);
        irCmdRepeat();
        bootDelayMs(IR_TEST_REPEAT_DELAY_MS);
    }
}

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

    ir_test();
}
