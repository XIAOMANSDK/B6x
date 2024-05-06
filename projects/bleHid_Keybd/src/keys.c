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
#include "hid_desc.h"

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

#define PA_LED0             PA08
#define PA_LED1             PA09
#define PA_BTN0             PA15
#define PA_BTN1             PA16
#define PA_BTN2             PA17

#define LED0                BIT(PA_LED0)
#define LED1                BIT(PA_LED1)
#define LEDS                (LED1 | LED0)

#define BTN0                BIT(PA_BTN0)
#define BTN1                BIT(PA_BTN1)
#define BTN2                BIT(PA_BTN2)
#define BTNS                (BTN0 | BTN1 | BTN2)


/*
 * FUNCTIONS
 ****************************************************************************************
 */

void hids_led_lock(uint8_t leds)
{
    if (leds & 0x01/*NUM_LOCK*/)
    {
        GPIO_DAT_CLR(LED0);
    }
    else
    {
        GPIO_DAT_SET(LED0);
    }
    
    if (leds & 0x02/*CAPS_LOCK*/)
    {
        GPIO_DAT_CLR(LED1);
    }
    else
    {
        GPIO_DAT_SET(LED1);
    }
}

uint8_t hid_keybd_send_report(uint8_t code)
{
    uint8_t ret = 0;
    
    uint8_t kyebd_report[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; //A
    
    kyebd_report[2] = code;
    
    ret = keybd_report_send(app_env.curidx, kyebd_report);

    return ret;
}

struct mouse_rep
{
    uint8_t btns;
    int16_t xoft;
    int16_t yoft;
    int8_t  wheel;
} __attribute__((packed));

uint8_t hid_mouse_send_report(int8_t x)
{
    uint8_t ret = 0;
    
    #if (HID_RPT_MOUSE)
    struct mouse_rep mouse_report;
    
    mouse_report.btns = 0;
    mouse_report.xoft = x;
    mouse_report.yoft = 0;
    mouse_report.wheel = 0;
    
    ret = mouse_report_send(app_env.curidx, (const uint8_t *)&mouse_report);
    #endif
    
    return ret;
}

void keys_init(void)
{
    GPIO_DIR_SET_HI(LEDS);

    GPIO_DAT_CLR(BTNS);
    GPIO_DIR_CLR(BTNS);

    iom_ctrl(PA_BTN0, IOM_PULLUP | IOM_INPUT);
    iom_ctrl(PA_BTN1, IOM_PULLUP | IOM_INPUT);
    iom_ctrl(PA_BTN2, IOM_PULLUP | IOM_INPUT);
}

void keys_scan(void)
{
    static uint32_t btn_lvl = BTNS;
    uint8_t ret = 0;
    uint32_t value = GPIO_PIN_GET() & BTNS;
    uint32_t chng = btn_lvl ^ value;
    btn_lvl = value;
    
    if (chng) {
        uint8_t code = 0;

        if ((chng & BTN0) && ((value & BTN0) == 0)) {
            code = 40;//HID_KEY_ENTER;
        }
        if ((chng & BTN1) && ((value & BTN1) == 0)) {
            code = 82;//HID_KEY_UP;
        }
        if ((chng & BTN2) && ((value & BTN2) == 0)) {
            code = 81;//HID_KEY_DOWN;
        }
        
        DEBUG("keys(val:%X,chng:%X,code:%d)\r\n", btn_lvl, chng, code);
        
        if (app_state_get() >= APP_CONNECTED)
        {
            ret = hid_keybd_send_report(code);
            if (ret == 0) {
                DEBUG("keys Fail(sta:%d)\r\n", ret);
            }
            
            if (code) {
                //GPIO_DAT_CLR(LED0);
                hid_mouse_send_report(10);
            }
            else {
                //GPIO_DAT_SET(LED0);
                hid_mouse_send_report(-10);
            }
        }
    }
}
