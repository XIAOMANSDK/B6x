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
#include "sftmr.h"
#include "ws2812.h"
#include "dbg.h"


/*
 * DEFINES
 ****************************************************************************************
 */



/*
 * FUNCTIONS
 ****************************************************************************************
 */
#define LOOP_PERIOD  _MS(10000)
static uint8_t test_mode_idx;
static tmr_tk_t ws2812_mode_loop_test(tmr_id_t id)
{
    (void)id;
    ws2812_play(test_mode_idx);
    test_mode_idx = (test_mode_idx + 1) % LED_MODE_MAX;

    return LOOP_PERIOD;
}

static void sysInit(void)
{
    // Todo config, if need
    SYS_CLK_ALTER();
}

static void devInit(void)
{
    uint16_t rsn = rstrsn();
    (void)rsn;
    iwdt_disable();

    rc32k_init();

    dbgInit();
    debug("Start(rsn:0x%X)...\r\n", rsn);

    sftmr_init();
    ws2812_init();

    test_mode_idx = LED_MODE_IDLE;

    sftmr_start(0, ws2812_mode_loop_test);
}

int main(void)
{
    sysInit();
    devInit();
    GLOBAL_INT_START();

    while (1)
    {
        sftmr_schedule();
    }
}
