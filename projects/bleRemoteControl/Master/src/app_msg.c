/**
 ****************************************************************************************
 *
 * @file app_msg.c
 *
 * @brief Application Messages Handler - Example
 *
 * < If want to modify it, recommend to copy the file to 'user porject'/src >
 ****************************************************************************************
 */

#include "app.h"
#include "drvs.h"
#include "usbd_hid.h"

#if (DBG_APP)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif

#define APP_INIT_TIMEOUT       30
/*
 * MESSAGE HANDLERS
 ****************************************************************************************
 */

/// SubTask Declaration, User add more...
extern APP_SUBTASK_HANDLER(gapm_msg);
extern APP_SUBTASK_HANDLER(gapc_msg);
extern APP_SUBTASK_HANDLER(gatt_msg);

enum app_custom_msg_id
{
    APP_INIT_TIMER = APP_BASE_MSG + 3,
};

void app_init_start(void)
{
    if (app_state_get() < APP_CONNECTED)
    {
        struct gap_bdaddr invalid_mac = {{0}, 0};
        if (memcmp((uint8_t *)&invalid_mac, scan_addr_list, 7))
        {
            DEBUG("init timer-->");
            app_start_initiating(scan_addr_list + 0);
        }
    }
}

#if (CFG_HW_TIMER)

#define SCAN_INV  _MS(APP_INIT_TIMEOUT)

static uint32_t app_init_timer(uint8_t id)
{
    app_init_start();

    return SCAN_INV;
}
#endif

void init_timer_start(void)
{   
    #if (CFG_HW_TIMER)
    sfTmrStart(SCAN_INV, app_init_timer);
    #else
    ke_timer_set(APP_INIT_TIMER, TASK_APP, APP_INIT_TIMEOUT);
    #endif

    DEBUG("init_timer_start");
}

void init_timer_stop(void)
{
    #if !(CFG_HW_TIMER)
    ke_timer_clear(APP_INIT_TIMER, TASK_APP);
    #endif
    DEBUG("init_timer_stop");
}

/**
 ****************************************************************************************
 * @brief SubTask Handler of Custom or Unknow Message. (__weak func)
 ****************************************************************************************
 */
//uint8_t mouse_data[4] = {0, 1, 0, 0};
__weak APP_SUBTASK_HANDLER(custom)
{
    #if !(CFG_HW_TIMER)
    if (msgid == APP_INIT_TIMER)
    {
        ke_timer_set(APP_INIT_TIMER, TASK_APP, APP_INIT_TIMEOUT);

        app_init_start();
    }
    else 
    #endif
    {
        uint16_t length = ke_param2msg(param)->param_len;
        DEBUG("Unknow MsgId:0x%X\r\n", msgid);
        debugHex((uint8_t *)param, length);
    }
    
    return (MSG_STATUS_FREE);
}

/**
 ****************************************************************************************
 * @brief Dispatch TASK_APP message to sub-handler.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] task_idx  Index of the receiving task instance.
 *
 * @return Handler of the message or NULL.
 ****************************************************************************************
 */
__TASKFN void* app_task_dispatch(msg_id_t msgid, uint8_t task_idx)
{
    msg_func_t handler = NULL;

    switch (MSG_TYPE(msgid))
    {
        case (TID_GAPM):
            handler = app_gapm_msg_handler;
            break;

        case (TID_GAPC):
            handler = app_gapc_msg_handler;
            break;

        #if (GATT_CLI)
        case (TID_GATT):
            handler = app_gatt_msg_handler;
            break;
        #endif

        default:
        {
            handler = app_custom_handler;
        } break;
    }

    return handler;
}
