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
#include "app_user.h"
#include "drvs.h"
#if (DBG_APP)
#include "dbg.h"
#define DEBUG(format, ...) debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif

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
    APP_WAIT_NEXT,
};

void app_init_start(void)
{
    if ((get_auto_conn()) && (app_state_get() < APP_CONNECTED))
    {
        const struct gap_bdaddr *list = scan_list_get();
        struct gap_bdaddr invalid_mac = { { {0} }, 0 };
        if (memcmp((uint8_t *)&invalid_mac, list, 7))
        {
            DEBUG("init timer-->");
            app_scan_action(ACTV_STOP);
            init_timer_stop();

            app_start_initiating(list);

            scan_list_reset();
        }
    }
}

void init_timer_start(void)
{
    if (get_auto_conn())
    {
        ke_timer_set(APP_INIT_TIMER, TASK_APP, APP_INIT_TIMEOUT);
    }

    DEBUG("init_timer_start");
}

void init_timer_stop(void)
{
    ke_timer_clear(APP_INIT_TIMER, TASK_APP);

    DEBUG("init_timer_stop");
}

void ota_ok_wait_next(void)
{
    // 3.5s
    ke_timer_set(APP_WAIT_NEXT, TASK_APP, 3500);
}

/**
 ****************************************************************************************
 * @brief SubTask Handler of Custom or Unknow Message. (__WEAK func)
 ****************************************************************************************
 */
__WEAK APP_SUBTASK_HANDLER(custom)
{
    (void)dest_id; (void)src_id;
    if (APP_INIT_TIMER == msgid)
    {
        ke_timer_set(APP_INIT_TIMER, TASK_APP, APP_INIT_TIMEOUT);

        app_init_start();
    }
    else if (APP_WAIT_NEXT == msgid)
    {
        scan_list_reset();
        init_timer_start();
        app_scan_action(ACTV_START);
    }
    else
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
__TASKFN void *app_task_dispatch(msg_id_t msgid, uint8_t task_idx)
{
    (void)task_idx;
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
        }
        break;
    }

    return handler;
}
