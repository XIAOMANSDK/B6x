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
#include "atcmd.h"

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

/**
 ****************************************************************************************
 * @brief API to Get Device Name, User Override!
 *
 * @param[in]     size   Length of name Buffer
 * @param[out] name   Pointer of name buffer
 *
 * @return Length of device name
 ****************************************************************************************
 */
uint8_t app_name_get(uint8_t size, uint8_t *name)
{
    uint8_t len = sys_config.name_len;
    
    if (len > size) len = size;

    memcpy(name, sys_config.name, len);
    return len;
}
