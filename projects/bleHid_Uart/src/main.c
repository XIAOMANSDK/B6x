/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Main Entry of the application.
 *
 ****************************************************************************************
 */

#include "bledef.h"
#include "drvs.h"
#include "app.h"
#include "sftmr.h"
#include "proto.h"

#include "dbg.h"

/*
 * DEFINES
 ****************************************************************************************
 */


/*
 * FUNCTIONS
 ****************************************************************************************
 */
void user_procedure(void);

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
    
    uint32_t read_mac = RD_32(FLASH_BASE + STORE_MAC_OFFSET);
    
    if (read_mac != 0xFFFFFFFF)
    {
        write32p(ble_dev_addr.addr, read_mac);
    }
    
    hids_led_lock(0x00);
    
    // init protocol parser
    proto_init(uart_proc);
    debug("Start(rsn:%X)...\r\n", rsn);

    sftmr_init();
    
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
        
        sftmr_schedule();
        
        // User's Procedure
        user_procedure();
    }
}
