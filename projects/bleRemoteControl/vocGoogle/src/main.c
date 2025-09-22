/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Main Entry of the application.
 *
 ****************************************************************************************
 */

#include "b6x.h"
#include "bledef.h"
#include "drvs.h"
#include "app.h"
#include "sftmr.h"
#include "leds.h"
#include "batt.h"
#include "keys.h"
#include "uartRb.h"
#include "dbg.h"
#include "app_user.h"
#include "ble_priv_data.h"

/*
 * DEFINES
 ****************************************************************************************
 */


/*
 * FUNCTIONS
 ****************************************************************************************
 */

extern void user_procedure(void);

static void sysInit(void)
{
    SYS_CLK_ALTER();
    iwdt_disable();

    rcc_ble_en();
    rcc_adc_en();

    puya_enter_dual_read();
    rcc_fshclk_set(FSH_CLK_DPSC42);
    core_release_io_latch();
}

static void devInit(void)
{
    uint16_t rsn = rstrsn();

    dbgInit();

    if (rsn & RSN_POR12_BK_BIT)
    {
        debug("\n[ptw] %s_%s_%s_%s_%s    %s %s\n\n", 
            CHIP_MODEL, SDK_VER, PROJ_NAME, HW_VERSION, PROJ_VERSION, 
            PROJ_BUILD_DATE, PROJ_BUILD_TIME);
    }

    if (rsn & (RSN_POR12_BK_BIT | RSN_PIN_RSTN_BIT | RSN_SYSRST_BIT))
    {
        ble_load_priv_data();
        debug("rsn:%X\n", rsn);
    }

    if (rsn != (RSN_BLE_WKUP_BIT | RSN_POR12_CORE_BIT))
    {
        debug("Start(rsn:%X)...\r\n", rsn);
    }
    
    keys_init();
    
    if ((rsn & RSN_POR12_CORE_BIT) != RSN_POR12_CORE_BIT)
    {
        last_sta = true;
        g_no_action_cnt = 0;
    }
    
    // Init BLE App
    app_init(rsn);
}

int main(void)
{
    sysInit();
    devInit();
    APBMISC->XOSC16M_CTRL.XOSC16M_CAP_TR = 0x1B;

    // Global Interrupt Enable
    GLOBAL_INT_START();

    // main loop
    while (1)
    {
        // Schedule Messages & Events
        ble_schedule();

        // User's Procedure
        user_procedure();
    }
}
