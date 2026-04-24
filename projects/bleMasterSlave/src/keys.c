/**
 ****************************************************************************************
 *
 * @file keys.c
 *
 * @brief Button and LED control for BLE multi-connection demo.
 *
 * BTN1 starts BLE scan, BTN2 initiates connection to first scan result.
 * LED1/LED2 indicate button press states.
 *
 ****************************************************************************************
 */

#include "b6x.h"
#include "drvs.h"
#include "app.h"

#if (DBG_KEYS)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<KEYS>" format "\r\n", ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#endif


/*
 * DEFINES
 ****************************************************************************************
 */

#define PA_LED1         (PA09)
#define PA_LED2         (PA10)
#define PA_BTN1         (PA16)
#define PA_BTN2         (PA17)

#define LED1            BIT(PA_LED1)
#define LED2            BIT(PA_LED2)
#define LEDS            (LED1 | LED2)

#define BTN1            BIT(PA_BTN1)
#define BTN2            BIT(PA_BTN2)
#define BTNS            (BTN1 | BTN2)


/*
 * FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 *
 * @brief Initialize GPIO for LEDs (output) and buttons (input with pull-up).
 *
 ****************************************************************************************
 */
void keys_init(void)
{
    GPIO_DIR_SET_HI(LEDS);

    GPIO_DAT_CLR(BTNS);
    GPIO_DIR_CLR(BTNS);

    iom_ctrl(PA_BTN1, IOM_PULLUP | IOM_INPUT);
    iom_ctrl(PA_BTN2, IOM_PULLUP | IOM_INPUT);
}

/**
 ****************************************************************************************
 *
 * @brief Poll button states and trigger BLE actions.
 *
 * BTN1 press: start BLE scan. BTN2 press: connect to first scan result.
 * Both LEDs reflect button active states.
 *
 * @note Must only be called when scan results are available for BTN2 action.
 *
 ****************************************************************************************
 */
void keys_scan(void)
{
    static uint32_t btn_lvl = BTNS;
    uint32_t value = GPIO_PIN_GET() & BTNS;
    uint32_t chng = btn_lvl ^ value;
    btn_lvl = value;

    if (chng)
    {
        if (chng & BTN1)
        {
            if ((value & BTN1) == 0)
            {
                GPIO_DAT_CLR(LED1);
                app_scan_action(ACTV_START);
            }
            else
            {
                GPIO_DAT_SET(LED1);
            }
        }

        if (chng & BTN2)
        {
            if ((value & BTN2) == 0)
            {
                GPIO_DAT_CLR(LED2);
                if (scan_cnt > 0)
                {
                    app_start_initiating(&scan_addr_list[0]);
                }
                else
                {
                    DEBUG("No scan result");
                }
            }
            else
            {
                GPIO_DAT_SET(LED2);
            }
        }

        DEBUG("keys(val:%" PRIX32 ",chng:%" PRIX32 ")\r\n", btn_lvl, chng);
    }
}
