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
#include "drvs.h"
#include "gapc_api.h"
#include "bledef.h"
#include "app_user.h"

#if (DBG_APP)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif

__RETENTION uint16_t g_no_action_cnt;
__RETENTION bool last_sta;
/*
 * MESSAGE HANDLERS
 ****************************************************************************************
 */

/// SubTask Declaration, User add more...
extern APP_SUBTASK_HANDLER(gapm_msg);
extern APP_SUBTASK_HANDLER(gapc_msg);
extern APP_SUBTASK_HANDLER(gatt_msg);
extern APP_SUBTASK_HANDLER(l2cc_msg);
extern APP_SUBTASK_HANDLER(mesh_msg);

void app_conn_param_update(bool key_change)
{
    if ((app_state_get() >= APP_BONDED) && (last_sta != key_change))
    {
        DEBUG("ls:%d,%d", last_sta, key_change);

        last_sta = key_change;

        if (!key_change)
        {
            DEBUG("update");
            ke_timer_set(APP_TIMER_NO_KEY_PRESS, TASK_APP, NOKEY_PRESS_UPDATE_PARAM);
        }
        else
        {
            g_no_action_cnt = 0;
            
            if (ke_timer_active(APP_TIMER_NO_KEY_PRESS, TASK_APP))
            {
                DEBUG("clear timer");
                ke_timer_clear(APP_TIMER_NO_KEY_PRESS, TASK_APP);
            }
            DEBUG("default");
            ble_latency_applied(false);
        }
    }
    
}
/**
 ****************************************************************************************
 * @brief SubTask Handler of Custom or Unknow Message. (__weak func)
 ****************************************************************************************
 */
__weak APP_SUBTASK_HANDLER(custom)
{
    switch (msgid)
    {
        #if (RC32K_CALIB_PERIOD)
        case APP_TIMER_RC32K_CORR:
        {
            DEBUG("rc32k_calib");
            rc32k_calib();
            ke_timer_set(APP_TIMER_RC32K_CORR, TASK_APP, RC32K_CALIB_PERIOD);

            if (++g_no_action_cnt > G_NO_ACTION_CNT)
            {
//                if (app_state_get() < APP_CONNECTED)
//                {
//                    keys_sleep();
//                }
            }
        } break;
        #endif
        
        case APP_TIMER_NO_KEY_PRESS:
        {
            ble_latency_applied(true);
            ke_timer_clear(APP_TIMER_NO_KEY_PRESS, TASK_APP);
        } break;
        
        case APP_TIMER_KEY_SCAN:
        {
            if (app_state_get() < APP_READY)
            {
                keys_proc();
                ke_timer_set(APP_TIMER_KEY_SCAN, TASK_APP, KEY_SCAN_PERIOD);
            }
            else
            {
                if (ke_timer_active(APP_TIMER_KEY_SCAN, TASK_APP))
                {
                    ke_timer_clear(APP_TIMER_KEY_SCAN, TASK_APP);
                }
            }
        } break;
        
        case APP_TIMER_KEY_ADV_DIR:
        {
            if (app_state_get() < APP_CONNECTED)
            {
                // 取消定向广播
                adv_dir_flag = false; // 取消定向广播
                app_adv_action(ACTV_RELOAD);            
            }
            ke_timer_clear(APP_TIMER_KEY_ADV_DIR, TASK_APP);
        } break;
        
        default:
        {
            uint16_t length = ke_param2msg(param)->param_len;
            DEBUG("Unknow MsgId:0x%X", msgid);
            debugHex((uint8_t *)param, length);
        } break;
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
        
        #if (L2CC_LECB)
        case (TID_L2CC):
            handler = app_l2cc_msg_handler;
            break;
        #endif

        #if (PRF_MESH)
        case TID_MESH:
            status = app_mesh_msg_handler;
            break;
        #endif

        default:
        {
            handler = app_custom_handler;
        } break;
    }

    return handler;
}
