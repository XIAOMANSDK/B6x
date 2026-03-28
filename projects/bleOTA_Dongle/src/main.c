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
#include "bledef.h"
#include "app.h"
#include "app_user.h"
#include "sftmr.h"
#include "uartRb.h"
#include "leds.h"
#include "dbg.h"

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
    SYS_CLK_ALTER();

    iwdt_disable();

    rcc_ble_en();

    rcc_adc_en();

    rcc_fshclk_set(FSH_CLK_DPSC42);

    puya_enter_dual_read();
}

static void devInit(void)
{
    uint16_t rsn = rstrsn();

    user_init();
#if (DBG_MODE)
    dbgInit();
    debug("Start(rsn:%X)...\r\n", rsn);
#endif

    sftmr_init();

    leds_init();

    uart1Rb_Init();

    // Init BLE App
    app_init(rsn);
}

int main(void)
{
    sysInit();
    devInit();

    // Global Interrupt Enable
    GLOBAL_INT_START();

    // main loop
    while (1)
    {
        // Schedule Messages & Events
        ble_schedule();

        user_procedure();

        sftmr_schedule();
    }
}
