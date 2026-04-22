/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Main Entry of the application - EXTI example
 *
 * @details
 * EXTI demo with two interrupt sources:
 * - PA_EXTI0 (PA16): falling edge trigger, pull-up input
 * - PA_EXTI1 (PA17): rising edge trigger, pull-down input
 * - Digital debounce enabled
 * - PA_RET_SEE (PA15): status indicator pin (scope probe)
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

#define PA_RET_SEE          (15)  ///< Status indicator pin - observe ISR duration
#define PA_EXTI0            (16)  ///< EXTI0 pin - falling edge trigger
#define PA_EXTI1            (17)  ///< EXTI1 pin - rising edge trigger

#define EXTI0_FLAG          (0x01)  ///< bit0: EXTI0 interrupt flag
#define EXTI1_FLAG          (0x02)  ///< bit1: EXTI1 interrupt flag


/*
 * GLOBALS
 ****************************************************************************************
 */

volatile uint8_t g_exti_irq_flag = 0;  ///< ISR-to-main flag: bit0=EXTI0, bit1=EXTI1


/*
 * FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief External interrupt service routine
 *
 * @details
 * Read RIF to identify source, clear ICR, set flag bits.
 * ISR only ORs flags; main loop consumes them.
 ****************************************************************************************
 */
void EXTI_IRQHandler(void)
{
    uint32_t irq_sta = EXTI->RIF.Word;
    uint32_t clr = 0;

    gpio_put_hi(PA_RET_SEE);

    if (irq_sta & EXTI_SRC(PA_EXTI0))
    {
        clr |= EXTI_SRC(PA_EXTI0);
        g_exti_irq_flag |= EXTI0_FLAG;
    }

    if (irq_sta & EXTI_SRC(PA_EXTI1))
    {
        clr |= EXTI_SRC(PA_EXTI1);
        g_exti_irq_flag |= EXTI1_FLAG;
    }

    EXTI->ICR.Word = clr;
    gpio_put_lo(PA_RET_SEE);
}

/**
 ****************************************************************************************
 * @brief EXTI test function
 *
 * @details
 * Configure GPIO pins, EXTI controller, debounce, NVIC, then poll flags.
 ****************************************************************************************
 */
static void exti_test(void)
{
    // GPIO pin configuration
    gpio_dir_output(PA_RET_SEE, OE_LOW);
    gpio_dir_input(PA_EXTI0, IE_UP);
    gpio_dir_input(PA_EXTI1, IE_DOWN);

    // EXTI controller configuration
    exti_init(EXTI_DBC(15, 4));
    exti_set(EXTI_FTS, EXTI_SRC(PA_EXTI0));
    exti_set(EXTI_RTS, EXTI_SRC(PA_EXTI1));
    exti_set(EXTI_DBE, EXTI_SRC(PA_EXTI0) | EXTI_SRC(PA_EXTI1));
    exti_set(EXTI_IER, EXTI_SRC(PA_EXTI0) | EXTI_SRC(PA_EXTI1));
    exti_set(EXTI_ICR, EXTI_SRC(PA_EXTI0) | EXTI_SRC(PA_EXTI1));
    NVIC_EnableIRQ(EXTI_IRQn);
    __enable_irq();

    while (1)
    {
        if (g_exti_irq_flag)
        {
            uint8_t flags = g_exti_irq_flag;
            g_exti_irq_flag = 0;
            debug("Trig: %X\r\n", flags);
        }
    }
}

/**
 ****************************************************************************************
 * @brief System initialization (template placeholder)
 ****************************************************************************************
 */
static void sys_init(void)
{
    // Todo config, if need
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
    iwdt_disable();
    dbgInit();
    debug("EXTI Test...\r\n");
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

    exti_test();
}
