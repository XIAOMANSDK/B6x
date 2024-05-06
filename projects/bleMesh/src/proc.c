/**
 ****************************************************************************************
 *
 * @file proc.c
 *
 * @brief user procedure.
 *
 ****************************************************************************************
 */

#include "b6x.h"
#include "bledef.h"
#include "drvs.h"
#include "mesh.h"


#if (DBG_PROC)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif


/*
 * DEFINES
 ****************************************************************************************
 */

#define GPIO_KEY0       GPIO15

static bool key0_press = false;

/*
 * FUNCTIONS
 ****************************************************************************************
 */

/// Simple detect key0 click
static bool key0_is_clicked()
{
    // current state (true: pressed, false: released)
    bool key0_state = ((GPIO_PIN_GET() & GPIO_KEY0) == 0);

    // click action: pressed, then released
    bool click = (key0_press & !key0_state);

    // update current state
    key0_press = key0_state;

    return click;
}

void user_procedure(void)
{
    if (key0_is_clicked())
    {
        // Restart proxy adv
        uint8_t status = ms_proxy_ctrl(MS_PROXY_ADV_NODE_START);

        if (status)
        {
            DEBUG("ms_proxy_ctrl fail(status:0x%X)", status);
        }
    }
    
    // add user more...
}
