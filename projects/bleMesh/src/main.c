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
#include "mesh.h"
#include "drvs.h"
#include "regs.h"
#include "app.h"
#include "sftmr.h"
#include "leds.h"
#include "uartRb.h"
#include "dbg.h"


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

    // more todo, eg. Set xosc cap

}

static void devInit(void)
{
    uint16_t rsn = rstrsn();

    dbgInit();
    debug("Start(rsn:%X)...\r\n", rsn);

    //uart1Rb_Init();

    // Init BLE App
    app_init(rsn);

    mesh_init(0);

    #if (LED_PLAY)
    sftmr_init();
    leds_init();
    leds_play(LED_FAST_BL);
    #endif //(LED_PLAY)

    // Init KEY0 & LED0~2 (@see B6-32D DKit-Board)
    gpio_dir_input(PA15, IE_UP);               // (PA15-->KEY-->GND)
    GPIO_DIR_SET_HI(GPIO08 | GPIO09 | GPIO10); // (VDD33-->LEDx-->PA08)
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

        #if (LED_PLAY)
        // SoftTimer Polling
        sftmr_schedule();
        #endif //(LED_PLAY)

        // User's Procedure
        user_procedure();
    }
}
