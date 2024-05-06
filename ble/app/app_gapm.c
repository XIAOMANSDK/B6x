/**
 ****************************************************************************************
 *
 * @file app_gapm.c
 *
 * @brief App SubTask of GAP Manager Message - Example
 *
 * < If want to modify it, recommend to copy the file to 'user porject'/src >
 ****************************************************************************************
 */

#include <stdint.h>
#include "bledef.h"
#include "app.h"

#if (DBG_GAPM)
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

APP_MSG_HANDLER(gapm_cmp_evt)
{
    if (param->operation == GAPM_RESET)
    {
        DEBUG("Reset(sta:0x%02X)", param->status);

        // Dev Config after reset
        app_conf_fsm(BLE_RESET);
    }
    else if (param->operation == GAPM_SET_DEV_CONFIG)
    {
        DEBUG("Dev Config(sta:0x%02X)", param->status);
        
        // Init profiles or activities after config
        app_conf_fsm(BLE_CONFIGURED);
    }
    #if (APP_ACTV_EN)
    else if (param->operation >= GAPM_CREATE_ADV_ACTIVITY)
    {
        DEBUG("Actv Evt(op:0x%02X,sta:0x%02X)", param->operation, param->status);
        
        app_actv_cmp_evt(param->operation, param->status);
    }
    #endif //(APP_ACTV_EN)
    else
    {
        DEBUG("Unknow Evt(op:0x%02X,sta:0x%02X)", param->operation, param->status);
    }
}

#if (APP_ACTV_EN)
APP_MSG_HANDLER(gapm_activity_created_ind)
{
    DEBUG("Actv Created(id:%d,typ:%d)", param->actv_idx, param->actv_type);
    
    app_actv_created_ind(param->actv_type, param->actv_idx);
}

APP_MSG_HANDLER(gapm_activity_stopped_ind)
{
    DEBUG("Actv Stopped(id:%d,typ:%d,rea:0x%02X)", param->actv_idx, param->actv_type, param->reason);
    
    app_actv_stopped_ind(param->actv_type, param->actv_idx, param->reason);
}

#if (BLE_EN_SCAN)
APP_MSG_HANDLER(gapm_ext_adv_report_ind)
{
    DEBUG("Scanned(id:%d,info:0x%02x,Addr{T:%d},Data{L:%d})", param->actv_idx, param->info,
        param->trans_addr.addr_type, param->length);
    debugHex((uint8_t *)(&param->trans_addr), sizeof(struct gap_bdaddr));
    debugHex(param->data, param->length);
    DEBUG("\r\n");
    
    app_actv_report_ind(param);
}
#endif //(BLE_EN_SCAN)
#endif //(APP_ACTV_EN)

#if (DBG_GAPM)
APP_MSG_HANDLER(gapm_scan_request_ind)
{
    DEBUG("Scan request(id:%d,Addr{T:%d})", param->actv_idx, param->trans_addr.addr_type);
    debugHex((uint8_t *)(&param->trans_addr), sizeof(struct gap_bdaddr));
}
#endif


/**
 ****************************************************************************************
 * @brief SubTask Handler of GAP manager Message.
 ****************************************************************************************
 */

APP_SUBTASK_HANDLER(gapm_msg)
{
    switch (msgid)
    {
        case GAPM_CMP_EVT:
        {
            APP_MSG_FUNCTION(gapm_cmp_evt);
        } break;

        #if (APP_ACTV_EN)
        case GAPM_ACTIVITY_CREATED_IND:
        {
            APP_MSG_FUNCTION(gapm_activity_created_ind);
        } break;
        
        case GAPM_ACTIVITY_STOPPED_IND:
        {
            APP_MSG_FUNCTION(gapm_activity_stopped_ind);
        } break;
        
        #if (BLE_EN_SCAN)
        case GAPM_EXT_ADV_REPORT_IND:
        {
            APP_MSG_FUNCTION(gapm_ext_adv_report_ind);
        } break;
        #endif //(BLE_EN_SCAN)
        
        #if (DBG_GAPM)
        case GAPM_SCAN_REQUEST_IND:
        {
            APP_MSG_FUNCTION(gapm_scan_request_ind);
        } break;
        #endif
        #endif //(APP_ACTV_EN)

        default:
        {
            DEBUG("Unknow MsgId:0x%X", msgid);
        } break;
    }

    return (MSG_STATUS_FREE);
}
