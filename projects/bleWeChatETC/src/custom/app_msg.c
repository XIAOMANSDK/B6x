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
#include "weChat.h"

#if (DBG_APP)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
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
extern APP_SUBTASK_HANDLER(l2cc_msg);
extern APP_SUBTASK_HANDLER(mesh_msg);

enum user_msg_id
{
    PRF_FEE7_IND_CFG = APP_BASE_MSG + 3,
};

void fee7_ind_cfg_timer_init(void)
{
    ke_timer_set(PRF_FEE7_IND_CFG, TASK_APP, 30);
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
        } break;
        #endif
        
        case PRF_FEE7_IND_CFG:
        {
            ble_req_auth();
            ke_timer_clear(PRF_FEE7_IND_CFG, TASK_APP);
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
