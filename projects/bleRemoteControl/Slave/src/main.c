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
    iwdt_disable();

    rcc_ble_en();
    rcc_adc_en();

    rcc_fshclk_set(FSH_CLK_DPSC42);
    core_release_io_latch();
}

static void devInit(void)
{
    uint16_t rsn = rstrsn();
    
    if (rsn & (RSN_POR12_BK_BIT | RSN_PIN_RSTN_BIT | RSN_SYSRST_BIT))
    {
        // 上电延迟5s, Jlink调试接管
        bootDelayMs(5000);
        
        // 上电进入PowerOFF
//        keys_sleep();

        iospc_rstpin(true);
        iospc_swdpin();
        
    }
    
    dbgInit();
    
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
    
    irInit();
    
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

        // User's Procedure
        user_procedure();
    }
}
