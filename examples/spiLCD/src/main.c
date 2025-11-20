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
#include "dbg.h"
#include "lcd.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/*
 * FUNCTIONS
 ****************************************************************************************
 */

uint32_t SystemCoreClock = SYS_GET_CLOCK();

void SysTick_Handler(void)
{
    GPIO_DAT_SET(GPIO02);
    GPIO_DAT_CLR(GPIO02);
}

static void sysInit(void)
{
    SYS_CLK_ALTER();
    GLOBAL_INT_START();
    puya_enter_dual_read();

    rcc_fshclk_set(FSH_CLK_DPSC64);

    /* set Priority for Systick Interrupt */
    NVIC_SetPriority (SysTick_IRQn, 1);
}

static void devInit(void)
{
    uint16_t rsn = rstrsn();

    iwdt_disable();

    GPIO_DIR_SET_LO(GPIO02 | GPIO03 | GPIO04);
    dbgInit();
    debug("Start(rsn:0x%X, clk:%d)...\r\n", rsn, rcc_sysclk_freq());
    lcd_dev_init();
}

static void userProc(void)
{
}

int main(void)
{
    sysInit();
    devInit();

    while (1)
    {
        userProc();
    }
}
