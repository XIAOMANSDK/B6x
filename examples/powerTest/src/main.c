/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Power management test - power off with IO wakeup detection
 *
 * @details
 * Test flow:
 * 1. On power-up: detect which IO pin triggered wakeup (if applicable)
 * 2. Wait for UART command to trigger power-off
 * 3. Configure IO wakeup sources and enter power-off (~1uA)
 *
 * On wakeup from IO, the code reads pin levels and XORs with the falling-edge
 * mask to identify the actual wakeup source.
 *
 ****************************************************************************************
 */

#include "drvs.h"
#include "regs.h"
#include "dbg.h"


/*
 * DEFINES
 ****************************************************************************************
 */

/// UART command to trigger power-off
#define CMD_POWER_OFF       0x66

/// All IO pins capable of wakeup: PA07 (UART RX), PA15/PA16/PA17 (KEY)
#define WKUP_IO_MASK        0x00038080UL

/// Falling-edge wakeup pins: PA07 (UART RX), PA15/PA16 (KEY)
#define WKUP_IO_FALLING     0x00018080UL

/// Delay before power-off to settle IO configuration
#define PWROFF_SETTLE_US    100

/*
 * VARIABLES
 ****************************************************************************************
 */

/// Retained across power-off in SRAM2; stores falling-edge wakeup IO mask
__RETENTION uint32_t waupIOFalling;

/*
 * FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief System initialization
 *
 * @details
 * Disable watchdog and release IO latch state retained from power-off.
 ****************************************************************************************
 */
static void sysInit(void)
{
    iwdt_disable();

    core_release_io_latch();
}

/**
 ****************************************************************************************
 * @brief Device initialization and wakeup source detection
 *
 * @details
 * - Read reset reason
 * - If IO wakeup: restore IO pins as GPIO input and identify the wakeup source
 * - Initialize debug interface and print wakeup info
 ****************************************************************************************
 */
static void devInit(void)
{
    uint16_t rsn = rstrsn();
    uint32_t waupIONow = 0;

    if (rsn & RSN_IO_WKUP_BIT)
    {
        /* Reconfigure wakeup IOs as GPIO input to read pin levels */
        for (uint8_t idx = 0; idx < PA_MAX; idx++)
        {
            if (BIT(idx) & WKUP_IO_MASK)
            {
                iom_ctrl(idx, IOM_SEL_GPIO | IOM_INPUT);
            }
        }

        waupIONow = GPIO_PIN_GET() & WKUP_IO_MASK;
    }

    dbgInit();
    debug("Start(rsn:0x%X)...\r\n", rsn);

    if (rsn & RSN_IO_WKUP_BIT)
    {
        debug("waupIONow1 [0x%06"PRIX32"]\r\n", waupIONow);

        /* XOR with falling-edge mask to identify the actual trigger */
        waupIONow ^= waupIOFalling;

        debug("waupIONow2 [0x%06"PRIX32"]\r\n", waupIONow);

        if (waupIONow)
        {
            for (uint8_t idx = 0; idx < PA_MAX; idx++)
            {
                if (BIT(idx) & waupIONow)
                {
                    debug("WAKE UP by IO PA[%02d]\r\n", idx);
                }
            }
        }
        else
        {
            debug("WAKE UP by IO PA[unknown]\r\n");
        }
    }
}

/**
 ****************************************************************************************
 * @brief Main entry
 *
 * @details
 * Wait for UART command, then enter power-off with IO wakeup enabled.
 * Blocking uart_getc keeps UART active (~1.5mA) for command reception.
 ****************************************************************************************
 */
int main(void)
{
    sysInit();
    devInit();

    while (1)
    {
        uint8_t cmd = uart_getc(0);
        debug("cmd:%x\r\n", cmd);

        switch (cmd)
        {
            case CMD_POWER_OFF:
            {
                debug("POWER OFF\r\n");
                waupIOFalling = WKUP_IO_FALLING;
                debug("waupIOALL [0x%06"PRIX32"]\r\n", WKUP_IO_MASK);
                debug("waupIOFalling [0x%06"PRIX32"]\r\n", waupIOFalling);

                wakeup_io_sw(WKUP_IO_MASK, WKUP_IO_FALLING);
                bootDelayUs(PWROFF_SETTLE_US);
                uart_wait(0);
                core_pwroff(WKUP_IO_EN_BIT | WKUP_IO_LATCH_N_BIT);
            } break;

            default:
            {
            } break;
        }
    }
}
