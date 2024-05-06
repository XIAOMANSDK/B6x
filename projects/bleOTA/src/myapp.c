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
#include "ota.h"

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

/**
 ****************************************************************************************
 * @brief API to Init Application, add ota_init
 *
 * @param[in] rsn   reset reason @see enum rst_src_bfs
 ****************************************************************************************
 */
void app_init(uint16_t rsn)
{
    heap_cfg_t heap;
    
    // Config Heap, resized with special mode
    heap.base[MEM_ENV] = BLE_HEAP_BASE;
    heap.size[MEM_ENV] = BLE_HEAP_ENV_SIZE;
    heap.base[MEM_MSG] = BLE_HEAP_BASE + BLE_HEAP_ENV_SIZE;
    heap.size[MEM_MSG] = BLE_HEAP_MSG_SIZE;

    ble_heap(&heap);

    // Init Ble Stack
    ble_init();

    // Init RF & Modem
    rfmdm_init();

    // Init App Task
    ble_app();

    ota_init();
}

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

