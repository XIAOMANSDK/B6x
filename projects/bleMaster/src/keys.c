/**
 ****************************************************************************************
 *
 * @file keys.c
 *
 * @brief keys operation.
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

void keys_init(void)
{
    GPIO_DIR_SET_HI(LEDS);
    
    GPIO_DAT_CLR(BTNS);
    GPIO_DIR_CLR(BTNS);

    iom_ctrl(PA_BTN1, IOM_PULLUP | IOM_INPUT);
    iom_ctrl(PA_BTN2, IOM_PULLUP | IOM_INPUT);
}

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
                app_start_initiating(&scan_addr_list[0]);
            }
            else
            {
                GPIO_DAT_SET(LED2);
            }
        }
        
        DEBUG("keys(val:%X,chng:%X,)\r\n", btn_lvl, chng);
    }
}
