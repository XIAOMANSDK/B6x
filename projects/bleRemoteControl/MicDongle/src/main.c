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
#include "sysdbg.h"
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
static void sysInit(void)
{
    rcc_sysclk_set(SYS_CLK);

    iwdt_disable();

    rcc_ble_en();

    rcc_adc_en();

    /***************************************/
    uint8_t fid = flashReadId() & 0xFF;

    if (fid == FSH_VID_BOYA)
    {
        rcc_fshclk_set(FSH_CLK_DPSC25);
        
        boya_flash_quad_mode();
        
        boya_enter_hpm();
    }
    else
    {
        rcc_fshclk_set(FSH_CLK_DPSC42);
        
        puya_enter_dual_read();        
    }
    /***************************************/

    // XOSC16M cap trim: eSOP8-VDD33+VDD12, ~2440MHz, +2.1KHz
    APBMISC->XOSC16M_CTRL.XOSC16M_CAP_TR = 0x20;
}

static void devInit(void)
{
    uint16_t rsn = rstrsn();
    uint32_t store_mac = RD_32(FLASH_BASE + BT_MAC_STORE_OFFSET);
    
#if (DBG_MODE)
    #if (DBG_UART_TXD == 19)
    iospc_rstpin(true);
    #endif
    dbgInit();
    debug("\r\n---Start(rsn:%X)...\r\n", rsn);
#endif

    if (store_mac != 0xFFFFFFFF)
    {
        write32p(ble_dev_addr.addr, store_mac);
    }
    
    // Init BLE App
    app_init(rsn);
    rf_pa_set(0x0C);  // RF PA gain: 0x0C for Dongle
    
    #if (CFG_USB)
    bootDelayMs(4);
    usbdInit();
    #endif
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
        
        #if (CFG_USB)
        usbd_mic_report();
        #endif
    }
}
