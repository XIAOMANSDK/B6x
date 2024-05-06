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
#include "drvs.h"
#include "uartRb.h"
#include "gfsk.h"
#include "dbg.h"

void uart_proc(void);

static void sysInit(void)
{
    rcc_adc_en();
    rcc_ble_en();
    
    rcc_fshclk_set(FSH_CLK_DPSC42);
}

static void devInit(void)
{
    uint16_t rsn = rstrsn();
    
    iwdt_disable();
    
    uart1Rb_Init();
    debug("Start(rsn:0x%X)...\r\n", rsn);

    rf_gfsk_init();
    rf_gfsk_tx_freq(2440);
}

int main(void)
{
    sysInit();
    devInit();
    
    // Global Interrupt Enable
    GLOBAL_INT_START();
    
    while (1)
    {
        uart_proc();
    }
}
