/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief 24BYJ48 stepper motor demo.
 *
 ****************************************************************************************
 */

#include "b6x.h"
#include "drvs.h"
#include "dbg.h"
#include "stepper.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/*
 * FUNCTIONS
 ****************************************************************************************
 */
static int32_t g_step = 4096;

static void stepTest(void)
{
    stepper_init();

    // Demo: clockwise 4096 steps (1 revolution) at 10 RPM, acceleration 200 steps/s^2
    stepper_move(g_step, 10, 200);

    while (1)
    {
        // Poll busy state
        if (!stepper_is_busy())
        {
            bootDelayMs(2000);

            g_step *= -1;
            stepper_move(g_step, 10, 200);

            // Halve step count each cycle, reset to 4096 when reaches 0
            g_step /= 2;
            if (!g_step) g_step = 4096;
        }
    }
}

static void sysInit(void)
{
    SYS_CLK_ALTER();
}

static void devInit(void)
{
    iwdt_disable();

    dbgInit();
    debug("stepperMotor Test...\r\n");
}

int main(void)
{
    sysInit();
    devInit();

    __enable_irq();
    stepTest();
}
