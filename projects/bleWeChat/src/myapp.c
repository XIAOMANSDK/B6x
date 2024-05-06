/**
 ****************************************************************************************
 *
 * @file myapp.c
 *
 * @brief User Application - Override func
 *
 ****************************************************************************************
 */

#include "app.h"
#include "bledef.h"
#include "drvs.h"
#include "leds.h"

#if (DBG_APP)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#endif


/*
 * DEFINES
 ****************************************************************************************
 */

const bd_addr_t ble_dev_addr_B = { BLE_ADDR_B };


/*
 * VARIABLES DEFINITIONS
 ****************************************************************************************
 */



/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

#if (LED_PLAY)
/**
 ****************************************************************************************
 * @brief API to Set State of Application, add leds Indication
 *
 * @param[in] state    new state
 ****************************************************************************************
 */
void app_state_set(uint8_t state)
{
    DEBUG("State(old:%d,new:%d)", app_state_get(), state);

    app_env.state = state;
    
    // Indication, User add more...
    if (state == APP_IDLE)
    {
        leds_play(LED_SLOW_BL);
    }
    else if (state == APP_READY)
    {
        leds_play(LED_FAST_BL);
    }
    else if (state == APP_CONNECTED)
    {
        leds_play(LED_CONT_ON);
    }
}
#endif //(LED_PLAY)

