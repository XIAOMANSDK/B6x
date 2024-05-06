/**
 ****************************************************************************************
 *
 * @file btns.c
 *
 * @brief Demo of Button Module via soft-timer. *User should override it*
 *
 ****************************************************************************************
 */

#include <stdint.h>
#include "string.h"
#include "sftmr.h"
#include "gpio.h"
#include "btns.h"


/*
 * USER CUSTOMIZE
 ****************************************************************************************
 */

/// iopad of Btns, total number NB_BTNS not excced 8.
const uint8_t PA_BTNS[] = 
{
    PA14,  // btn0
    PA15,  // btn1
};

#define IO_BTNS             (BIT(PA14) | BIT(PA15)) // *same PA_BTNS*
#define NB_BTNS             sizeof(PA_BTNS)

/// iopad input mode(pull-up or pull-down)
#define IE_MODE             (IOM_INPUT | IOM_PULLUP)
#define IN_PRESS            (0) // pull-up 1 -> 0

/// time for events
#define SCAN_INTV           _MS(20)
#define TCNT_DCLK           (_MS( 200) / SCAN_INTV)
#define TCNT_LONG           (_MS(1000) / SCAN_INTV)
#define TCNT_LLONG          (_MS(3000) / SCAN_INTV)


/*
 * DEFINES
 ****************************************************************************************
 */

#define BTN(n)              (1 << (n)) // Bit of Btn index

#if !defined(TRIG_BTN)
#define TRIG_BTN            (1) // Triggle Event: Press or Release
#endif
#if !defined(DCLK_BTN)
#define DCLK_BTN            (1) // Double Click
#endif
#if !defined(LONG_BTN)
#define LONG_BTN            (1) // Long Press
#endif

typedef struct {
    uint16_t tcnt :12;
    uint16_t event: 4;
} btn_sta_t;

typedef struct btn_env_tag {
    btn_func_t func;    // event call
    uint8_t    tmrid;   // softTimer ID
    uint8_t    level;   // gpio val
    uint8_t    keys;    // real keys
    uint8_t    trig;    // trigger
    btn_sta_t  state[NB_BTNS]; 
} btn_env_t;

/// global variables
static btn_env_t btn_env;


/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

static __forceinline void btns_io_init(void)
{
    GPIO_DIR_CLR(IO_BTNS); // OE=0
    
    // enable input mode
    for (uint8_t n = 0; n < NB_BTNS; n++)
    {
        iom_ctrl(PA_BTNS[n], IE_MODE);
    }
}

static __forceinline uint8_t btns_get_level(void)
{
    uint8_t level = 0;
    uint32_t iostat = GPIO_PIN_GET() & IO_BTNS;

    for (uint8_t n = 0; n < NB_BTNS; n++)
    {
        if (((iostat >> PA_BTNS[n]) & 0x01) == IN_PRESS)
        {
            level |= BTN(n);
        }
    }
    
    return level;
}

static void btns_scan(void)
{
    uint8_t level, keys;
    
    // debounce
    level = btns_get_level(); // gpio value
    keys  = (level & btn_env.level) | ((level ^ btn_env.level) & btn_env.keys); // real value

    // trigger
    btn_env.trig  = keys ^ btn_env.keys;
    btn_env.keys  = keys;
    btn_env.level = level;
        
    for (uint8_t n = 0; n < NB_BTNS; n++)
    {
        if (btn_env.keys & BTN(n)) // key press
        {
            if (btn_env.trig & BTN(n)) // on trig
            {
                #if (TRIG_BTN)
                btn_env.func(n, BTN_PRESS);
                #endif
                
                #if (DCLK_BTN)
                if (btn_env.state[n].event == BTN_CLICK)
                {
                    if (btn_env.state[n].tcnt < TCNT_DCLK)
                    {
                        btn_env.state[n].event = BTN_DCLICK;
                    }
                }
                else
                #endif
                {
                    btn_env.state[n].event = BTN_CLICK;
                }
                btn_env.state[n].tcnt = 0;
            }
            else
            {
                if (btn_env.state[n].event != BTN_IDLE)
                {
                    btn_env.state[n].tcnt++;
                    #if (LONG_BTN)
                    if (btn_env.state[n].tcnt == TCNT_LONG)
                    {
                        btn_env.state[n].event = BTN_LONG;
                        btn_env.func(n, BTN_LONG);
                    }
                    else if (btn_env.state[n].tcnt >= TCNT_LLONG)
                    {
                        btn_env.state[n].event = BTN_IDLE;
                        btn_env.func(n, BTN_LLONG);
                    }
                    #endif
                }
            }
        }
        else // key release
        {
            if (btn_env.trig & BTN(n)) // on trig
            {
                #if (TRIG_BTN)
                btn_env.func(n, BTN_RELEASE);
                #endif
                
                #if (DCLK_BTN)
                if (btn_env.state[n].event == BTN_DCLICK)
                {
                    btn_env.state[n].event = BTN_IDLE;
                    btn_env.func(n, BTN_DCLICK);
                }
                #endif
                btn_env.state[n].tcnt = 0;
            }
            else
            {
                if ((btn_env.state[n].event == BTN_CLICK) && (++btn_env.state[n].tcnt >= TCNT_DCLK))
                {
                    btn_env.state[n].event = BTN_IDLE;
                    btn_env.func(n, BTN_CLICK);
                }
            }
        }
    }
}

static tmr_tk_t btns_timer_handler(tmr_id_t id)
{
    // peroid scan
    btns_scan();
    
    return SCAN_INTV;
}

void btns_conf(btn_func_t hdl)
{
    // clear curr timer
    if (btn_env.tmrid != TMR_ID_NONE)
    {
        sftmr_clear(btn_env.tmrid);
        
        memset(&btn_env, 0, sizeof(btn_env_t));
        //btn_env.tmrid = TMR_ID_NONE;
    }
    
    if (hdl)
    {
        // update handler, start timer
        btn_env.func  = hdl;
        btn_env.tmrid = sftmr_start(SCAN_INTV, btns_timer_handler);
    }
}

void btns_init(void)
{
    // init gpio
    btns_io_init();

    // init btn_env
    memset(&btn_env, 0, sizeof(btn_env_t));
    //btn_env.tmrid = TMR_ID_NONE;
}
